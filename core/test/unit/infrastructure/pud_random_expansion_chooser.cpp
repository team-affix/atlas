#include <algorithm>
#include <map>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_random_expansion_chooser.hpp"

using ::testing::_;
using ::testing::NiceMock;

namespace {

struct MockGetBodyGoals {
    MOCK_METHOD(std::vector<pud_body_goal>, body_goals, (pud_derivation_id));
};

std::vector<pud_body_goal> body_goals_with(const std::vector<pud_body_goal_id>& ids) {
    std::vector<pud_body_goal> goals;
    for (pud_body_goal_id id : ids)
        goals.push_back(pud_body_goal{.id = id, .goal = nullptr});
    return goals;
}

using test_chooser_t = pud_random_expansion_chooser<NiceMock<MockGetBodyGoals>>;

struct PudRandomExpansionChooserTest : public ::testing::Test {
    NiceMock<MockGetBodyGoals> get_body_goals;
    std::mt19937 rng{7};
    test_chooser_t chooser{get_body_goals, rng};
    std::map<pud_derivation_id, std::vector<pud_body_goal_id>> goals{
        {pud_derivation_id{3}, {0, 2}},
        {pud_derivation_id{8}, {1}},
        {pud_derivation_id{21}, {0, 4, 5}},
        {pud_derivation_id{40}, {2, 3}}};
    std::set<pud_derivation_id> frontier{
        pud_derivation_id{3}, pud_derivation_id{8}, pud_derivation_id{21}, pud_derivation_id{40}};

    void SetUp() override {
        ON_CALL(get_body_goals, body_goals(_)).WillByDefault([this](pud_derivation_id derivation) {
            return body_goals_with(goals[derivation]);
        });
    }
};

}

TEST_F(PudRandomExpansionChooserTest, OnlyOpenGoalIsAlwaysChosen) {
    goals[pud_derivation_id{9}] = {6};
    for (int draw = 0; draw < 50; ++draw)
        EXPECT_EQ(chooser.choose_expansion({pud_derivation_id{9}}), (pud_expansion_choice{pud_derivation_id{9}, 6}));
}

TEST_F(PudRandomExpansionChooserTest, OnlyTheChosenDerivationsGoalsAreRead) {
    goals[pud_derivation_id{9}] = {6};
    EXPECT_CALL(get_body_goals, body_goals(pud_derivation_id{9}));
    chooser.choose_expansion({pud_derivation_id{9}});
}

TEST_F(PudRandomExpansionChooserTest, ChoiceIsAlwaysAnOpenGoalOfAnOfferedDerivation) {
    for (int draw = 0; draw < 200; ++draw) {
        const pud_expansion_choice choice = chooser.choose_expansion(frontier);
        ASSERT_TRUE(frontier.contains(choice.derivation));
        const std::vector<pud_body_goal_id>& open_goals = goals.at(choice.derivation);
        EXPECT_NE(std::find(open_goals.begin(), open_goals.end(), choice.goal), open_goals.end());
    }
}

TEST_F(PudRandomExpansionChooserTest, DerivationsAreChosenRoughlyUniformly) {
    std::map<pud_derivation_id, int> counts;
    for (int draw = 0; draw < 4000; ++draw)
        ++counts[chooser.choose_expansion(frontier).derivation];
    EXPECT_EQ(counts.size(), frontier.size());
    for (const auto& [derivation, count] : counts)
        EXPECT_GT(count, 800) << derivation.value;
}

TEST_F(PudRandomExpansionChooserTest, GoalsAreChosenRoughlyUniformly) {
    std::map<pud_body_goal_id, int> counts;
    for (int draw = 0; draw < 3000; ++draw)
        ++counts[chooser.choose_expansion({pud_derivation_id{21}}).goal];
    EXPECT_EQ(counts.size(), 3u);
    for (const auto& [goal, count] : counts)
        EXPECT_GT(count, 800) << goal;
}

TEST_F(PudRandomExpansionChooserTest, SameSeedGivesTheSameChoices) {
    std::mt19937 other_rng{7};
    test_chooser_t other{get_body_goals, other_rng};
    for (int draw = 0; draw < 100; ++draw)
        EXPECT_EQ(chooser.choose_expansion(frontier), other.choose_expansion(frontier));
}

TEST_F(PudRandomExpansionChooserTest, NoOpenDerivationIsAPreconditionViolation) {
    EXPECT_THROW(chooser.choose_expansion({}), std::logic_error);
}

TEST_F(PudRandomExpansionChooserTest, DerivationWithoutGoalsIsAPreconditionViolation) {
    goals[pud_derivation_id{9}] = {};
    EXPECT_THROW(chooser.choose_expansion({pud_derivation_id{9}}), std::logic_error);
}
