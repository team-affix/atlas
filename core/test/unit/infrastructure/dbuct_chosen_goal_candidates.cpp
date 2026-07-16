// dbuct_chosen_goal_candidates: set/try_get and frame undo.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_chosen_goal_candidates.hpp"
#include "value_objects/lineage.hpp"

struct DbuctChosenGoalCandidatesTest : public ::testing::Test {
    dbuct_chosen_goal_candidates chosen;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
};

TEST_F(DbuctChosenGoalCandidatesTest, EmptyTryGetIsNullopt) {
    EXPECT_EQ(chosen.try_get(&gl0), std::nullopt);
}

TEST_F(DbuctChosenGoalCandidatesTest, SetThenTryGet) {
    chosen.push_frame();
    chosen.set(&gl0, 7);
    EXPECT_EQ(chosen.try_get(&gl0), std::optional<rule_id>{7});
}

TEST_F(DbuctChosenGoalCandidatesTest, OverwriteSameGoal) {
    chosen.push_frame();
    chosen.set(&gl0, 7);
    chosen.set(&gl0, 9);
    EXPECT_EQ(chosen.try_get(&gl0), std::optional<rule_id>{9});
}

TEST_F(DbuctChosenGoalCandidatesTest, GoalsAreIndependent) {
    chosen.push_frame();
    chosen.set(&gl0, 1);
    chosen.set(&gl1, 2);
    EXPECT_EQ(chosen.try_get(&gl0), std::optional<rule_id>{1});
    EXPECT_EQ(chosen.try_get(&gl1), std::optional<rule_id>{2});
}

TEST_F(DbuctChosenGoalCandidatesTest, PopFrameUndoesSet) {
    chosen.push_frame();
    chosen.set(&gl0, 1);
    chosen.push_frame();
    chosen.set(&gl0, 2);
    chosen.pop_frame();
    EXPECT_EQ(chosen.try_get(&gl0), std::optional<rule_id>{1});
}
