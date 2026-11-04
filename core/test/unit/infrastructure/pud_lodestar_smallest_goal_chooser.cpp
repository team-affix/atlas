#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_smallest_goal_chooser.hpp"
#include "pud_test_node.hpp"

using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id), (const));
};

using goals_t = immer::map<pud_body_goal_id, const expr*>;

struct PudLodestarSmallestGoalChooserTest : public ::testing::Test {
    NiceMock<MockGetNodeGoals> node_goals;
    pud_lodestar_smallest_goal_chooser<NiceMock<MockGetNodeGoals>> chooser{node_goals};
};

}

TEST_F(PudLodestarSmallestGoalChooserTest, TheSmallestOpenGoalIdIsChosenWhateverTheMapsOrder) {
    const pud_goal_list goals{.goals = goals_t{}.set(9, nullptr).set(4, nullptr).set(12, nullptr), .next_id = 13};
    ON_CALL(node_goals, get(pud_test_node(5))).WillByDefault(ReturnRef(goals));
    EXPECT_EQ(chooser.choose(pud_test_node(5)), 4u);
}

TEST_F(PudLodestarSmallestGoalChooserTest, ALeafsOnlyOpenGoalIsChosen) {
    const pud_goal_list goals{.goals = goals_t{}.set(7, nullptr), .next_id = 8};
    ON_CALL(node_goals, get(pud_test_node(5))).WillByDefault(ReturnRef(goals));
    EXPECT_EQ(chooser.choose(pud_test_node(5)), 7u);
}

TEST_F(PudLodestarSmallestGoalChooserTest, ChoosingForALeafWithNoOpenGoalBreaksTheInvariant) {
    const pud_goal_list goals{.goals = {}, .next_id = 8};
    ON_CALL(node_goals, get(pud_test_node(5))).WillByDefault(ReturnRef(goals));
    EXPECT_THROW(chooser.choose(pud_test_node(5)), std::logic_error);
}
