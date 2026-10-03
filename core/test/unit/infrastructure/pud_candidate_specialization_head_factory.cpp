#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_candidate_specialization_head_factory.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

using child_iter = std::vector<const pud_node*>::const_iterator;

struct MockTryAdd {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_add_head, (pud_query_position<int>));
};
struct MockAdvance {
    MOCK_METHOD((pud_query_frame<int, child_iter>), advance_head, (pud_mhws_head_id));
};
struct MockForkWitness {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_fork_head, (pud_mhws_head_id, int));
};
struct MockLeaf {
    MOCK_METHOD(bool, check_leaf, (const pud_node*));
};
struct MockChildren {
    MOCK_METHOD((const std::vector<const pud_node*>&), get, (const pud_node*));
};
struct MockPropagate {
    MOCK_METHOD(std::optional<int>, propagate, (int, const pud_node*));
};

using test_factory_t = pud_candidate_specialization_head_factory<
    int, child_iter, MockTryAdd, MockAdvance, MockForkWitness, MockLeaf, MockChildren, MockPropagate>;

struct PudCandidateSpecializationHeadFactoryTest : public ::testing::Test {
    NiceMock<MockTryAdd> try_add;
    NiceMock<MockAdvance> advance;
    NiceMock<MockForkWitness> fork_witness;
    NiceMock<MockLeaf> leaves;
    NiceMock<MockChildren> children;
    NiceMock<MockPropagate> propagate;
    test_factory_t factory{try_add, advance, fork_witness, leaves, children, propagate};
    pud_node root{};
    pud_node child{};
    std::vector<const pud_node*> root_children{&child};
};

TEST_F(PudCandidateSpecializationHeadFactoryTest, NoWitnessMeansNoJustification) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(children, get(&root)).WillRepeatedly(ReturnRef(root_children));
    EXPECT_CALL(propagate, propagate(_, _)).WillRepeatedly(Return(2));
    EXPECT_CALL(try_add, try_add_head(_)).WillRepeatedly(Return(std::nullopt));
    auto head = factory.make(pud_query_position<int>{.handle = 1, .node = &root});
    EXPECT_FALSE(head.resume().has_value());
}

TEST_F(PudCandidateSpecializationHeadFactoryTest, LeafRootIsASelfWitness) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto head = factory.make(pud_query_position<int>{.handle = 1, .node = &root});
    auto found = head.resume();
    ASSERT_TRUE(found.has_value());
    auto* self = std::get_if<pud_candidate_self_witness>(&found->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, &root);
}
