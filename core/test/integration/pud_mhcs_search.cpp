#if 0
#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_candidate_specialization_head_factory.hpp"
#include "infrastructure/pud_candidate_specialization_head_forker.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_witness_search_head_factory.hpp"
#include "infrastructure/pud_witness_search_head_forker.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

using child_iter = std::vector<const pud_node*>::const_iterator;

struct MockCheckLeaf {
    MOCK_METHOD(bool, check_leaf, (const pud_node*));
};

struct MockGetChildren {
    MOCK_METHOD((const std::vector<const pud_node*>&), get, (const pud_node*));
};

struct MockPropagate {
    MOCK_METHOD(std::optional<int>, propagate, (int, const pud_node*));
};

struct MockCallSite {
    MOCK_METHOD(size_t, get, (const pud_node*));
};

using witness_head_t = pud_witness_search_head<
    int, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite>;
using witness_factory_t = pud_witness_search_head_factory<
    int, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite>;
using witness_forker_t = pud_witness_search_head_forker<
    int, child_iter, MockCheckLeaf, MockGetChildren, MockPropagate, MockCallSite>;
using mhws_t = pud_mhws<int, child_iter, witness_head_t, witness_factory_t, witness_forker_t>;

using candidate_head_t = pud_candidate_specialization_head<
    int, child_iter, mhws_t, mhws_t, mhws_t, MockCheckLeaf, MockGetChildren, MockPropagate>;
using candidate_factory_t = pud_candidate_specialization_head_factory<
    int, child_iter, mhws_t, mhws_t, mhws_t, MockCheckLeaf, MockGetChildren, MockPropagate>;
using candidate_forker_t = pud_candidate_specialization_head_forker<
    int, child_iter, mhws_t, mhws_t, mhws_t, MockCheckLeaf, MockGetChildren, MockPropagate>;
using mhcs_t = pud_mhcs<int, candidate_head_t, candidate_factory_t, candidate_forker_t>;

struct PudMhcsSearchIntegrationTest : public ::testing::Test {
    NiceMock<MockCheckLeaf> leaves;
    NiceMock<MockGetChildren> children;
    NiceMock<MockPropagate> propagate;
    NiceMock<MockCallSite> call_sites;
    witness_factory_t witness_factory{leaves, children, propagate, call_sites};
    witness_forker_t witness_forker;
    mhws_t witnesses{witness_factory, witness_forker};
    candidate_factory_t candidate_factory{witnesses, witnesses, witnesses, leaves, children, propagate};
    candidate_forker_t candidate_forker;
    mhcs_t candidates{candidate_factory, candidate_forker};
    std::unordered_map<const pud_node*, std::vector<const pud_node*>> sequences;
    pud_node root{};
    pud_node only{};
    pud_node left{};
    pud_node right{};
    pud_node later{};
    pud_node sibling{};

    void SetUp() override {
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(children, get(_)).WillByDefault([&](const pud_node* node) -> const std::vector<const pud_node*>& {
            return sequences.at(node);
        });
        ON_CALL(propagate, propagate(_, _)).WillByDefault([](int handle, const pud_node*) {
            return std::optional<int>{handle + 1};
        });
    }
};

TEST_F(PudMhcsSearchIntegrationTest, SingleWitnessIsNotAChoicePoint) {
    sequences[&root] = {&only};
    sequences[&only] = {};
    EXPECT_CALL(leaves, check_leaf(&only)).WillRepeatedly(Return(true));
    auto id = candidates.try_add_head(pud_query_position<int>{.handle = 1, .node = &root});
    ASSERT_TRUE(id.has_value());
    auto gone = candidates.invalidate_leaf(&only);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
    pud_mhws_head_id absent = 99;
    EXPECT_FALSE(candidates.witness_refuted(absent).has_value());
}

TEST_F(PudMhcsSearchIntegrationTest, RefutingOneWitnessWithNoReplacementDropsTheOther) {
    sequences[&root] = {&left, &right};
    sequences[&left] = {};
    sequences[&right] = {};
    EXPECT_CALL(leaves, check_leaf(&left)).WillRepeatedly(Return(true));
    EXPECT_CALL(leaves, check_leaf(&right)).WillRepeatedly(Return(true));
    auto id = candidates.try_add_head(pud_query_position<int>{.handle = 1, .node = &root});
    ASSERT_TRUE(id.has_value());
    EXPECT_CALL(leaves, check_leaf(&left)).WillRepeatedly(Return(false));
    EXPECT_CALL(propagate, propagate(_, _)).WillRepeatedly(Return(std::nullopt));
    auto dropped = witnesses.invalidate_leaf(&left);
    ASSERT_EQ(dropped.size(), 1u);
    auto lost = candidates.witness_refuted(dropped[0]);
    ASSERT_TRUE(lost.has_value());
    EXPECT_EQ(*lost, *id);
    auto other = witnesses.invalidate_leaf(&right);
    ASSERT_EQ(other.size(), 1u);
    EXPECT_FALSE(candidates.witness_refuted(other[0]).has_value());
}

TEST_F(PudMhcsSearchIntegrationTest, TwoSelfWitnessesOnlyTheOneWithoutANewJustificationFails) {
    sequences[&root] = {};
    sequences[&sibling] = {};
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto first = candidates.try_add_head(pud_query_position<int>{.handle = 1, .node = &root});
    auto second = candidates.try_add_head(pud_query_position<int>{.handle = 2, .node = &root});
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    sequences[&root] = {&sibling};
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(false));
    EXPECT_CALL(leaves, check_leaf(&sibling)).WillOnce(Return(false)).WillRepeatedly(Return(true));
    auto gone = candidates.invalidate_leaf(&root);
    ASSERT_EQ(gone.size(), 1u);
    auto later = candidates.invalidate_leaf(&sibling);
    ASSERT_EQ(later.size(), 1u);
}

TEST_F(PudMhcsSearchIntegrationTest, ForkThatCannotPropagateLeavesTheOriginal) {
    sequences[&root] = {};
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto id = candidates.try_add_head(pud_query_position<int>{.handle = 1, .node = &root});
    ASSERT_TRUE(id.has_value());
    EXPECT_CALL(propagate, propagate(50, &root)).WillOnce(Return(std::nullopt));
    EXPECT_FALSE(candidates.try_fork_head(*id, 50).has_value());
    auto gone = candidates.invalidate_leaf(&root);
    ASSERT_EQ(gone.size(), 1u);
    EXPECT_EQ(gone[0], *id);
}

TEST_F(PudMhcsSearchIntegrationTest, RemoveLeafRootLeavesLaterInvalidateEmpty) {
    sequences[&root] = {};
    EXPECT_CALL(leaves, check_leaf(&root)).WillRepeatedly(Return(true));
    auto id = candidates.try_add_head(pud_query_position<int>{.handle = 1, .node = &root});
    ASSERT_TRUE(id.has_value());
    candidates.remove_head(*id);
    EXPECT_TRUE(candidates.invalidate_leaf(&root).empty());
}
#endif
