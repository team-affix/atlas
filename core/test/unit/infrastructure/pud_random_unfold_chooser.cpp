#include <algorithm>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_random_unfold_chooser.hpp"

using ::testing::_;
using ::testing::NiceMock;

namespace {

struct MockIterateBodyGoals {
    MOCK_METHOD((coroutine<pud_open_goal, void>), iterate_body_goals, (pud_derivation_id));
};

coroutine<pud_open_goal, void> yield_goals(std::vector<pud_body_goal_id> goals) {
    for (pud_body_goal_id goal : goals)
        co_yield pud_open_goal{.goal = goal, .candidate_count = 2};
}

using test_chooser_t = pud_random_unfold_chooser<NiceMock<MockIterateBodyGoals>>;

struct PudRandomUnfoldChooserTest : public ::testing::Test {
    NiceMock<MockIterateBodyGoals> iterate_body_goals;
    std::mt19937 rng{7};
    test_chooser_t chooser{iterate_body_goals, rng};
    std::map<pud_derivation_id, std::vector<pud_body_goal_id>> goals{
        {pud_derivation_id{3}, {0, 2}},
        {pud_derivation_id{8}, {1}},
        {pud_derivation_id{21}, {0, 4, 5}},
        {pud_derivation_id{40}, {2, 3}}};
    std::set<pud_derivation_id> frontier{
        pud_derivation_id{3}, pud_derivation_id{8}, pud_derivation_id{21}, pud_derivation_id{40}};

    void SetUp() override {
        ON_CALL(iterate_body_goals, iterate_body_goals(_)).WillByDefault([this](pud_derivation_id derivation) {
            return yield_goals(goals[derivation]);
        });
    }
};

}

TEST_F(PudRandomUnfoldChooserTest, OnlyOpenGoalIsAlwaysChosen) {
    goals[pud_derivation_id{9}] = {6};
    for (int draw = 0; draw < 50; ++draw)
        EXPECT_EQ(chooser.choose_unfold({pud_derivation_id{9}}), (pud_unfold_choice{pud_derivation_id{9}, 6}));
}

TEST_F(PudRandomUnfoldChooserTest, OnlyTheChosenDerivationsGoalsAreIterated) {
    goals[pud_derivation_id{9}] = {6};
    EXPECT_CALL(iterate_body_goals, iterate_body_goals(pud_derivation_id{9}));
    chooser.choose_unfold({pud_derivation_id{9}});
}

TEST_F(PudRandomUnfoldChooserTest, ChoiceIsAlwaysAnOpenGoalOfAnOfferedDerivation) {
    for (int draw = 0; draw < 200; ++draw) {
        const pud_unfold_choice choice = chooser.choose_unfold(frontier);
        ASSERT_TRUE(frontier.contains(choice.derivation));
        const std::vector<pud_body_goal_id>& open_goals = goals.at(choice.derivation);
        EXPECT_NE(std::find(open_goals.begin(), open_goals.end(), choice.goal), open_goals.end());
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
        ++counts[chooser.choose_unfold({pud_derivation_id{21}}).goal];
    EXPECT_EQ(counts.size(), 3u);
    for (const auto& [goal, count] : counts)
        EXPECT_GT(count, 800) << goal;
}

TEST_F(PudRandomUnfoldChooserTest, SameSeedGivesTheSameChoices) {
    std::mt19937 other_rng{7};
    test_chooser_t other{iterate_body_goals, other_rng};
    for (int draw = 0; draw < 100; ++draw)
        EXPECT_EQ(chooser.choose_unfold(frontier), other.choose_unfold(frontier));
}

TEST_F(PudRandomUnfoldChooserTest, NoOpenDerivationIsAPreconditionViolation) {
    EXPECT_THROW(chooser.choose_unfold({}), std::logic_error);
}

TEST_F(PudRandomUnfoldChooserTest, DerivationWithoutGoalsIsAPreconditionViolation) {
    goals[pud_derivation_id{9}] = {};
    EXPECT_THROW(chooser.choose_unfold({pud_derivation_id{9}}), std::logic_error);
}
