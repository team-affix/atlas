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
