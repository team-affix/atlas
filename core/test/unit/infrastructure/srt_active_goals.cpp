// srt_active_goals: batch insert/link/flush, leaf-only membership, iteration, clear.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "value_objects/lineage.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

template<typename Yield>
std::vector<Yield> collect_yields(coroutine<Yield, void>& sm) {
    std::vector<Yield> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

}  // namespace

struct SrtActiveGoalsTest : public ::testing::Test {
    srt_active_goals goals;
    goal_lineage parent{nullptr, 0};
    goal_lineage parent2{nullptr, 1};
    goal_lineage child0{nullptr, 2};
    goal_lineage child1{nullptr, 3};
    goal_lineage child2{nullptr, 4};
    goal_lineage child3{nullptr, 5};
};

TEST_F(SrtActiveGoalsTest, EmptyInitially) {
    EXPECT_TRUE(goals.empty());
    EXPECT_EQ(goals.active_goals_size(), 0u);
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), IsEmpty());
}

TEST_F(SrtActiveGoalsTest, InsertMakesIsolatedGoalActive) {
    goals.insert_active_goal(&child0);
    EXPECT_FALSE(goals.empty());
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_EQ(goals.active_goals_size(), 1u);
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), UnorderedElementsAre(&child0));
}

TEST_F(SrtActiveGoalsTest, DuplicateInsertThrows) {
    goals.insert_active_goal(&child0);
    EXPECT_THROW(goals.insert_active_goal(&child0), std::logic_error);
}

TEST_F(SrtActiveGoalsTest, ClearActiveGoalsEmptiesRegistry) {
    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.clear_active_goals();
    EXPECT_TRUE(goals.empty());
    EXPECT_FALSE(goals.is_active_goal(&child0));
    EXPECT_FALSE(goals.is_active_goal(&child1));
}

TEST_F(SrtActiveGoalsTest, ParentNotActiveAfterBatchLink) {
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_TRUE(goals.is_active_goal(&child1));
    EXPECT_FALSE(goals.is_active_goal(&parent));
    EXPECT_EQ(goals.active_goals_size(), 2u);
}

TEST_F(SrtActiveGoalsTest, IsolatedInsertSurvivesFlushWithoutLink) {
    goals.insert_active_goal(&child0);
    goals.flush_srt_goal_batch();
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_EQ(goals.active_goals_size(), 1u);
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), UnorderedElementsAre(&child0));
}

TEST_F(SrtActiveGoalsTest, BatchLinkWiresInFlightGoalsUnderParent) {
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();
    auto child_sm = goals.iterate_child_goals(&parent);
    EXPECT_THAT(collect_yields(child_sm), UnorderedElementsAre(&child0, &child1));
}

TEST_F(SrtActiveGoalsTest, SecondBatchAfterFlushCanLinkNewGoals) {
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&parent2);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&child2);
    goals.insert_active_goal(&child3);
    goals.link_srt_goal_batch_parent(&parent2);
    goals.flush_srt_goal_batch();
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_TRUE(goals.is_active_goal(&child1));
    EXPECT_TRUE(goals.is_active_goal(&child2));
    EXPECT_TRUE(goals.is_active_goal(&child3));
    EXPECT_FALSE(goals.is_active_goal(&parent));
    EXPECT_FALSE(goals.is_active_goal(&parent2));
    EXPECT_EQ(goals.active_goals_size(), 4u);
}

TEST_F(SrtActiveGoalsTest, DeepThreeLevelTreeLinksAndIteratesHierarchy) {
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&parent2);
    goals.insert_active_goal(&child2);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.link_srt_goal_batch_parent(&parent2);
    goals.flush_srt_goal_batch();
    EXPECT_FALSE(goals.is_active_goal(&parent));
    EXPECT_FALSE(goals.is_active_goal(&parent2));
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_TRUE(goals.is_active_goal(&child1));
    EXPECT_TRUE(goals.is_active_goal(&child2));
    EXPECT_EQ(goals.active_goals_size(), 3u);
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), UnorderedElementsAre(&parent));
    auto top_children_sm = goals.iterate_child_goals(&parent);
    EXPECT_THAT(collect_yields(top_children_sm), UnorderedElementsAre(&parent2, &child2));
    auto mid_children_sm = goals.iterate_child_goals(&parent2);
    EXPECT_THAT(collect_yields(mid_children_sm), UnorderedElementsAre(&child0, &child1));
}

TEST_F(SrtActiveGoalsTest, IterateRootGoalsYieldsCurrentRoots) {
    goals.insert_active_goal(&child0);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&child1);
    goals.flush_srt_goal_batch();
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), UnorderedElementsAre(&child0, &child1));
}

TEST_F(SrtActiveGoalsTest, IterateChildGoalsYieldsLinkedChildrenOnly) {
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&child2);
    goals.flush_srt_goal_batch();
    auto child_sm = goals.iterate_child_goals(&parent);
    EXPECT_THAT(collect_yields(child_sm), UnorderedElementsAre(&child0, &child1));
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), UnorderedElementsAre(&parent, &child2));
}

TEST_F(SrtActiveGoalsTest, LinkWithEmptyInFlightRemovesParent) {
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();
    EXPECT_NO_THROW(goals.link_srt_goal_batch_parent(&parent));
    EXPECT_FALSE(goals.is_active_goal(&parent));
    EXPECT_TRUE(goals.empty());
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), IsEmpty());
}

TEST_F(SrtActiveGoalsTest, LinkWithEmptyInFlightLeavesSiblingActiveUnderGrandparent) {
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&parent2);
    goals.insert_active_goal(&child2);
    goals.insert_active_goal(&child3);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();
    EXPECT_NO_THROW(goals.link_srt_goal_batch_parent(&parent2));
    EXPECT_FALSE(goals.is_active_goal(&parent2));
    EXPECT_TRUE(goals.is_active_goal(&child2));
    EXPECT_TRUE(goals.is_active_goal(&child3));
    EXPECT_FALSE(goals.is_active_goal(&parent));
    EXPECT_EQ(goals.active_goals_size(), 2u);
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), UnorderedElementsAre(&parent));
    auto child_sm = goals.iterate_child_goals(&parent);
    EXPECT_THAT(collect_yields(child_sm), UnorderedElementsAre(&child2, &child3));
}

TEST_F(SrtActiveGoalsTest, LinkWithEmptyInFlightRemovesNonRootParent) {
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&parent2);
    goals.insert_active_goal(&child2);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();
    EXPECT_NO_THROW(goals.link_srt_goal_batch_parent(&parent2));
    EXPECT_FALSE(goals.is_active_goal(&parent2));
    EXPECT_FALSE(goals.is_active_goal(&parent));
    EXPECT_TRUE(goals.is_active_goal(&child2));
    EXPECT_EQ(goals.active_goals_size(), 1u);
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), UnorderedElementsAre(&child2));
}

TEST_F(SrtActiveGoalsTest, IsActiveGoalFalseForNeverInserted) {
    EXPECT_FALSE(goals.is_active_goal(&child0));
}

TEST_F(SrtActiveGoalsTest, DuplicateInsertAfterGroundingRemovalSucceeds) {
    goals.insert_active_goal(&child0);
    goals.flush_srt_goal_batch();
    goals.link_srt_goal_batch_parent(&child0);
    goals.flush_srt_goal_batch();
    EXPECT_NO_THROW(goals.insert_active_goal(&child0));
    goals.flush_srt_goal_batch();
    EXPECT_TRUE(goals.is_active_goal(&child0));
}

TEST_F(SrtActiveGoalsTest, ClearActiveGoalsClearsPendingInFlightBatch) {
    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.clear_active_goals();
    EXPECT_TRUE(goals.empty());
    goals.insert_active_goal(&child0);
    goals.flush_srt_goal_batch();
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_FALSE(goals.is_active_goal(&child1));
}

TEST_F(SrtActiveGoalsTest, MultipleInsertsInOneBatchWithoutLinkStayIsolated) {
    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.flush_srt_goal_batch();
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_TRUE(goals.is_active_goal(&child1));
    EXPECT_EQ(goals.active_goals_size(), 2u);
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), UnorderedElementsAre(&child0, &child1));
}

TEST_F(SrtActiveGoalsTest, SingleChildBatchLinkLeavesOnlyChildActive) {
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&child0);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_FALSE(goals.is_active_goal(&parent));
    EXPECT_EQ(goals.active_goals_size(), 1u);
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), UnorderedElementsAre(&child0));
}

TEST_F(SrtActiveGoalsTest, IterateChildGoalsThrowsForIsolatedRoot) {
    goals.insert_active_goal(&child0);
    goals.flush_srt_goal_batch();
    auto child_sm = goals.iterate_child_goals(&child0);
    EXPECT_THROW(collect_yields(child_sm), std::out_of_range);
}

TEST_F(SrtActiveGoalsTest, EmptyMatchesActiveGoalsSizeZero) {
    EXPECT_TRUE(goals.empty());
    EXPECT_EQ(goals.active_goals_size(), 0u);
    goals.insert_active_goal(&child0);
    goals.flush_srt_goal_batch();
    EXPECT_FALSE(goals.empty());
    EXPECT_EQ(goals.active_goals_size(), 1u);
}
