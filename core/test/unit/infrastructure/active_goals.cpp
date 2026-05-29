// Active goal registry: insertion, erasure, membership, iteration, size, and empty
// invariants. The set must not contain duplicates, must reflect erase, and iteration
// must visit exactly the inserted goals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/active_goals.hpp"
#include "value_objects/lineage.hpp"

using ::testing::Contains;
using ::testing::IsEmpty;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<const goal_lineage*> collect_goals(const active_goals& ag) {
    std::vector<const goal_lineage*> out;
    auto sm = ag.iterate_active_goals();
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

}  // namespace

struct ActiveGoalsTest : public ::testing::Test {
    active_goals goals;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
};

TEST_F(ActiveGoalsTest, EmptyInitially) {
    EXPECT_TRUE(goals.empty());
    EXPECT_EQ(goals.active_goals_size(), 0u);
    EXPECT_THAT(collect_goals(goals), IsEmpty());
}

TEST_F(ActiveGoalsTest, InsertMakesGoalActive) {
    goals.insert_active_goal(&gl0);
    EXPECT_FALSE(goals.empty());
    EXPECT_TRUE(goals.is_active_goal(&gl0));
    EXPECT_EQ(goals.active_goals_size(), 1u);
}

TEST_F(ActiveGoalsTest, DuplicateInsertIsIdempotent) {
    goals.insert_active_goal(&gl0);
    goals.insert_active_goal(&gl0);
    EXPECT_EQ(goals.active_goals_size(), 1u);
}

TEST_F(ActiveGoalsTest, EraseRemovesGoal) {
    goals.insert_active_goal(&gl0);
    goals.erase_active_goal(&gl0);
    EXPECT_FALSE(goals.is_active_goal(&gl0));
    EXPECT_TRUE(goals.empty());
}

TEST_F(ActiveGoalsTest, IterateYieldsAllInsertedGoals) {
    goals.insert_active_goal(&gl0);
    goals.insert_active_goal(&gl1);
    EXPECT_THAT(collect_goals(goals), UnorderedElementsAre(&gl0, &gl1));
}

TEST_F(ActiveGoalsTest, EraseOneGoalLeavesOther) {
    goals.insert_active_goal(&gl0);
    goals.insert_active_goal(&gl1);
    goals.erase_active_goal(&gl0);
    EXPECT_THAT(collect_goals(goals), UnorderedElementsAre(&gl1));
    EXPECT_FALSE(goals.is_active_goal(&gl0));
    EXPECT_TRUE(goals.is_active_goal(&gl1));
}

TEST_F(ActiveGoalsTest, ClearActiveGoalsEmptiesRegistry) {
    goals.insert_active_goal(&gl0);
    goals.insert_active_goal(&gl1);
    goals.clear_active_goals();
    EXPECT_TRUE(goals.empty());
    EXPECT_THAT(collect_goals(goals), IsEmpty());
}
