// Random-access active goal registry: insertion, erasure, membership, select, size,
// empty invariants, and swap-and-pop index stability.

#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <set>
#include "infrastructure/ra_active_goals.hpp"
#include "value_objects/lineage.hpp"

using ::testing::IsEmpty;
using ::testing::UnorderedElementsAre;

namespace {

std::set<const goal_lineage*> collect_via_select(const ra_active_goals& ag) {
    std::set<const goal_lineage*> out;
    for (size_t i = 0; i < ag.active_goals_size(); ++i)
        out.insert(ag.select(i));
    return out;
}

}  // namespace

struct RaActiveGoalsTest : public ::testing::Test {
    ra_active_goals goals;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
    goal_lineage gl2{nullptr, 2};
};

TEST_F(RaActiveGoalsTest, EmptyInitially) {
    EXPECT_TRUE(goals.empty());
    EXPECT_EQ(goals.active_goals_size(), 0u);
    EXPECT_THAT(collect_via_select(goals), IsEmpty());
}

TEST_F(RaActiveGoalsTest, InsertMakesGoalActive) {
    goals.insert_active_goal(&gl0);
    EXPECT_FALSE(goals.empty());
    EXPECT_TRUE(goals.is_active_goal(&gl0));
    EXPECT_EQ(goals.active_goals_size(), 1u);
    EXPECT_EQ(goals.select(0), &gl0);
}

TEST_F(RaActiveGoalsTest, DuplicateInsertThrows) {
    goals.insert_active_goal(&gl0);
    EXPECT_THROW(goals.insert_active_goal(&gl0), std::logic_error);
}

TEST_F(RaActiveGoalsTest, EraseRemovesGoal) {
    goals.insert_active_goal(&gl0);
    goals.erase_active_goal(&gl0);
    EXPECT_FALSE(goals.is_active_goal(&gl0));
    EXPECT_TRUE(goals.empty());
}

TEST_F(RaActiveGoalsTest, SelectYieldsAllInsertedGoals) {
    goals.insert_active_goal(&gl0);
    goals.insert_active_goal(&gl1);
    EXPECT_THAT(collect_via_select(goals), UnorderedElementsAre(&gl0, &gl1));
}

TEST_F(RaActiveGoalsTest, EraseOneGoalLeavesOther) {
    goals.insert_active_goal(&gl0);
    goals.insert_active_goal(&gl1);
    goals.erase_active_goal(&gl0);
    EXPECT_THAT(collect_via_select(goals), UnorderedElementsAre(&gl1));
    EXPECT_FALSE(goals.is_active_goal(&gl0));
    EXPECT_TRUE(goals.is_active_goal(&gl1));
}

TEST_F(RaActiveGoalsTest, SwapAndPopKeepsRemainingGoalsSelectable) {
    goals.insert_active_goal(&gl0);
    goals.insert_active_goal(&gl1);
    goals.insert_active_goal(&gl2);
    goals.erase_active_goal(&gl1);
    EXPECT_EQ(goals.active_goals_size(), 2u);
    EXPECT_THAT(collect_via_select(goals), UnorderedElementsAre(&gl0, &gl2));
}

TEST_F(RaActiveGoalsTest, ClearActiveGoalsEmptiesRegistry) {
    goals.insert_active_goal(&gl0);
    goals.insert_active_goal(&gl1);
    goals.clear_active_goals();
    EXPECT_TRUE(goals.empty());
    EXPECT_THAT(collect_via_select(goals), IsEmpty());
}
