#include <algorithm>
#include <map>
#include <random>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_random_unfold_chooser.hpp"

namespace {
using frontier_t = std::map<pud_derivation_id, std::vector<pud_body_goal_id>>;

struct PudRandomUnfoldChooserTest : public ::testing::Test {
    std::mt19937 rng{7};
    pud_random_unfold_chooser chooser{rng};
    frontier_t frontier{
        {pud_derivation_id{3}, {0, 2}},
        {pud_derivation_id{8}, {1}},
        {pud_derivation_id{21}, {0, 4, 5}},
        {pud_derivation_id{40}, {2, 3}}};
};

}

TEST_F(PudRandomUnfoldChooserTest, OnlyOpenGoalIsAlwaysChosen) {
    for (int draw = 0; draw < 50; ++draw)
        EXPECT_EQ(chooser.choose_unfold({{pud_derivation_id{9}, {6}}}), (pud_unfold_choice{pud_derivation_id{9}, 6}));
}

TEST_F(PudRandomUnfoldChooserTest, ChoiceIsAlwaysAnOfferedGoalOfAnOfferedDerivation) {
    for (int draw = 0; draw < 200; ++draw) {
        const pud_unfold_choice choice = chooser.choose_unfold(frontier);
        ASSERT_TRUE(frontier.contains(choice.derivation));
        const std::vector<pud_body_goal_id>& goals = frontier.at(choice.derivation);
        EXPECT_NE(std::find(goals.begin(), goals.end(), choice.goal), goals.end());
    }
}

TEST_F(PudRandomUnfoldChooserTest, DerivationsAreChosenRoughlyUniformly) {
    std::map<pud_derivation_id, int> counts;
    for (int draw = 0; draw < 4000; ++draw)
        ++counts[chooser.choose_unfold(frontier).derivation];
    EXPECT_EQ(counts.size(), frontier.size());
    for (const auto& [derivation, count] : counts)
        EXPECT_GT(count, 800) << derivation.value;
}

TEST_F(PudRandomUnfoldChooserTest, GoalsAreChosenRoughlyUniformly) {
    std::map<pud_body_goal_id, int> counts;
    for (int draw = 0; draw < 3000; ++draw)
        ++counts[chooser.choose_unfold({{pud_derivation_id{21}, {0, 4, 5}}}).goal];
    EXPECT_EQ(counts.size(), 3u);
    for (const auto& [goal, count] : counts)
        EXPECT_GT(count, 800) << goal;
}

TEST_F(PudRandomUnfoldChooserTest, SameSeedGivesTheSameChoices) {
    std::mt19937 other_rng{7};
    pud_random_unfold_chooser other{other_rng};
    for (int draw = 0; draw < 100; ++draw)
        EXPECT_EQ(chooser.choose_unfold(frontier), other.choose_unfold(frontier));
}

TEST_F(PudRandomUnfoldChooserTest, NoOpenDerivationIsAPreconditionViolation) {
    EXPECT_THROW(chooser.choose_unfold({}), std::logic_error);
}

TEST_F(PudRandomUnfoldChooserTest, DerivationWithoutGoalsIsAPreconditionViolation) {
    EXPECT_THROW(chooser.choose_unfold({{pud_derivation_id{9}, {}}}), std::logic_error);
}
