#if 0
#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_candidate_specialization_head_forker.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

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

using test_head_t = pud_candidate_specialization_head<
    int, child_iter, MockTryAdd, MockAdvance, MockForkWitness, MockLeaf, MockChildren, MockPropagate>;

struct PudCandidateSpecializationHeadForkerTest : public ::testing::Test {
    NiceMock<MockTryAdd> try_add;
    NiceMock<MockAdvance> advance;
    NiceMock<MockForkWitness> fork_witness;
    NiceMock<MockLeaf> leaves;
    NiceMock<MockChildren> children;
    NiceMock<MockPropagate> propagate;
    pud_candidate_specialization_head_forker<
        int, child_iter, MockTryAdd, MockAdvance, MockForkWitness, MockLeaf, MockChildren, MockPropagate> forker;
    pud_node root{};

    test_head_t make_head() {
        return test_head_t{
            try_add, advance, fork_witness, leaves, children, propagate,
            pud_query_position<int>{.handle = 1, .node = &root}};
    }
};

TEST_F(PudCandidateSpecializationHeadForkerTest, ForkThatCannotFollowHasNoJustification) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto head = make_head();
    ASSERT_TRUE(head.resume().has_value());
    EXPECT_CALL(propagate, propagate(50, &root)).WillOnce(Return(std::nullopt));
    auto forked = forker.fork(head, 50);
    EXPECT_FALSE(forked.resume().has_value());
}

TEST_F(PudCandidateSpecializationHeadForkerTest, ForkOfLeafIsSelfWitnessUnderNewHandle) {
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto head = make_head();
    EXPECT_CALL(propagate, propagate(50, &root)).WillOnce(Return(60));
    auto forked = forker.fork(head, 50);
    auto found = forked.resume();
    ASSERT_TRUE(found.has_value());
    auto* self = std::get_if<pud_candidate_self_witness>(&found->justification);
    ASSERT_NE(self, nullptr);
    EXPECT_EQ(self->node, &root);
    EXPECT_EQ(found->query_handle, 60);
}
#endif
