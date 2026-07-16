// chosen_goal_candidates: set/try_get, overwrite, independence, and clear.

#include <gtest/gtest.h>
#include "infrastructure/chosen_goal_candidates.hpp"
#include "value_objects/lineage.hpp"

struct ChosenGoalCandidatesTest : public ::testing::Test {
    chosen_goal_candidates chosen;
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
};

TEST_F(ChosenGoalCandidatesTest, EmptyTryGetIsNullopt) {
    EXPECT_EQ(chosen.try_get(&gl0), std::nullopt);
}

TEST_F(ChosenGoalCandidatesTest, SetThenTryGet) {
    chosen.set(&gl0, 7);
    EXPECT_EQ(chosen.try_get(&gl0), std::optional<rule_id>{7});
}

TEST_F(ChosenGoalCandidatesTest, OverwriteSameGoal) {
    chosen.set(&gl0, 7);
    chosen.set(&gl0, 9);
    EXPECT_EQ(chosen.try_get(&gl0), std::optional<rule_id>{9});
}

TEST_F(ChosenGoalCandidatesTest, GoalsAreIndependent) {
    chosen.set(&gl0, 1);
    chosen.set(&gl1, 2);
    EXPECT_EQ(chosen.try_get(&gl0), std::optional<rule_id>{1});
    EXPECT_EQ(chosen.try_get(&gl1), std::optional<rule_id>{2});
}

TEST_F(ChosenGoalCandidatesTest, ClearRemovesAllChoices) {
    chosen.set(&gl0, 1);
    chosen.set(&gl1, 2);
    chosen.clear();
    EXPECT_EQ(chosen.try_get(&gl0), std::nullopt);
    EXPECT_EQ(chosen.try_get(&gl1), std::nullopt);
}
