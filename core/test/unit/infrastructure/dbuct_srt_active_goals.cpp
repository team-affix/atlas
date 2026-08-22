// dbuct_srt_active_goals: insert/link/flush membership and frame undo.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_srt_active_goals.hpp"
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

} // namespace

struct DbuctSrtActiveGoalsTest : public ::testing::Test {
    dbuct_srt_active_goals goals;
    goal_lineage parent{nullptr, 0};
    goal_lineage child0{nullptr, 2};
    goal_lineage child1{nullptr, 3};
    goal_lineage child2{nullptr, 5};
    goal_lineage child3{nullptr, 6};
    goal_lineage orphan{nullptr, 4};
};

TEST_F(DbuctSrtActiveGoalsTest, EmptyInitially) {
    EXPECT_TRUE(goals.empty());
    EXPECT_EQ(goals.active_goals_size(), 0u);
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), IsEmpty());
}

TEST_F(DbuctSrtActiveGoalsTest, InsertMakesIsolatedGoalActive) {
    goals.push_frame();
    goals.insert_active_goal(&child0);
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_EQ(goals.active_goals_size(), 1u);
    auto root_sm = goals.iterate_root_goals();
    EXPECT_THAT(collect_yields(root_sm), UnorderedElementsAre(&child0));
}

TEST_F(DbuctSrtActiveGoalsTest, BatchLinkActivatesChildrenNotParent) {
    goals.push_frame();
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();
    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_TRUE(goals.is_active_goal(&child1));
    EXPECT_FALSE(goals.is_active_goal(&parent));
    auto child_sm = goals.iterate_child_goals(&parent);
    EXPECT_THAT(collect_yields(child_sm), UnorderedElementsAre(&child0, &child1));
}

TEST_F(DbuctSrtActiveGoalsTest, PopFrameRestoresMembership) {
    goals.push_frame();
    goals.insert_active_goal(&child0);
    goals.push_frame();
    goals.insert_active_goal(&child1);
    EXPECT_EQ(goals.active_goals_size(), 2u);
    goals.pop_frame();
    EXPECT_TRUE(goals.is_active_goal(&child0));
    EXPECT_FALSE(goals.is_active_goal(&child1));
    EXPECT_EQ(goals.active_goals_size(), 1u);
}

// An activation that fails partway leaves the children it already inserted in the
// pending batch, since srt_subgoals_activator returns before it links or flushes.
// Under dbuct nothing clears the store between episodes, so the frame pop is the
// only thing that discards them. The batch has no accessor, but it IS the child
// set of the next link, so linking a fresh batch reveals whatever it still holds.
// Two children are needed: a one-child link series-reduces the parent away.
TEST_F(DbuctSrtActiveGoalsTest, PopFrameDiscardsUnflushedBatch) {
    goals.push_frame();
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();

    goals.push_frame();
    goals.insert_active_goal(&orphan);
    goals.pop_frame();

    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();

    EXPECT_FALSE(goals.is_active_goal(&orphan));
    EXPECT_FALSE(goals.is_active_goal(&parent));
    EXPECT_EQ(goals.active_goals_size(), 2u);
    EXPECT_EQ(goals.get_parent_goal(&child0), &parent);
    auto child_sm = goals.iterate_child_goals(&parent);
    EXPECT_THAT(collect_yields(child_sm), UnorderedElementsAre(&child0, &child1));
}

// The mirror case: a batch that completed normally inside the frame. Popping it
// must leave the parent genuinely reusable, which is what backtracking relies on
// when it retries a goal against a different rule. Relinking with a DIFFERENT pair
// of children is what gives this teeth -- replaying the same two would still pass
// against a pop that left the old child set behind.
TEST_F(DbuctSrtActiveGoalsTest, PopFrameAfterCompletedBatchLetsParentTakeDifferentChildren) {
    goals.push_frame();
    goals.insert_active_goal(&parent);
    goals.flush_srt_goal_batch();

    goals.push_frame();
    goals.insert_active_goal(&child0);
    goals.insert_active_goal(&child1);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();
    goals.pop_frame();

    EXPECT_TRUE(goals.is_active_goal(&parent));
    EXPECT_EQ(goals.active_goals_size(), 1u);

    goals.insert_active_goal(&child2);
    goals.insert_active_goal(&child3);
    goals.link_srt_goal_batch_parent(&parent);
    goals.flush_srt_goal_batch();

    EXPECT_FALSE(goals.is_active_goal(&parent));
    EXPECT_FALSE(goals.is_active_goal(&child0));
    EXPECT_FALSE(goals.is_active_goal(&child1));
    EXPECT_EQ(goals.active_goals_size(), 2u);
    auto child_sm = goals.iterate_child_goals(&parent);
    EXPECT_THAT(collect_yields(child_sm), UnorderedElementsAre(&child2, &child3));
}
