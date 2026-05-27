// Active goal registry: insertion, erasure, membership, iteration, size, and empty
// invariants. The set must not contain duplicates, must reflect erase, and iteration
// must visit exactly the inserted goals.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/active_goals.hpp"
#include "../../../core/hpp/value_objects/lineage.hpp"

using ::testing::Contains;
using ::testing::IsEmpty;
using ::testing::SizeIs;
using ::testing::UnorderedElementsAre;

namespace {

std::vector<const goal_lineage*> collect_goals(const active_goals& ag) {
    std::vector<const goal_lineage*> out;
    auto sm = ag.iterate_active_goals();
    while (!sm.done()) {
        if (auto gl = sm.resume())
            out.push_back(*gl);
    }
    return out;
}

}  // namespace

struct ActiveGoalsTest : public ::testing::Test {
    active_goals goals;
    expr e0{expr::var{0}};
    expr e1{expr::var{1}};
    goal_lineage gl0{nullptr, &e0};
    goal_lineage gl1{nullptr, &e1};
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
