#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_unit_goal_chooser.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockFindUnitGoal {
    MOCK_METHOD(std::optional<pud_body_goal_id>, find_unit_goal, (pud_global_node_id));
};
struct MockChooseGoal {
    MOCK_METHOD(pud_body_goal_id, choose, (pud_global_node_id));
};

// The other chooser picks goal 4 of leaf 5.
struct PudLodestarUnitGoalChooserTest : public ::testing::Test {
    NiceMock<MockFindUnitGoal> unit_goals;
    NiceMock<MockChooseGoal> fallback;
    pud_lodestar_unit_goal_chooser<NiceMock<MockFindUnitGoal>, NiceMock<MockChooseGoal>> chooser{unit_goals, fallback};

    void SetUp() override {
        ON_CALL(unit_goals, find_unit_goal(_)).WillByDefault(Return(std::nullopt));
        ON_CALL(fallback, choose(pud_test_node(5))).WillByDefault(Return(4));
    }
};

}

TEST_F(PudLodestarUnitGoalChooserTest, ALeafsUnitGoalIsChosenWithoutAskingTheOtherChooser) {
    ON_CALL(unit_goals, find_unit_goal(pud_test_node(5))).WillByDefault(Return(9));
    EXPECT_CALL(fallback, choose(_)).Times(0);
    EXPECT_EQ(chooser.choose(pud_test_node(5)), 9u);
}

TEST_F(PudLodestarUnitGoalChooserTest, ALeafWithoutAUnitGoalGetsTheOtherChoosersGoal) {
    EXPECT_EQ(chooser.choose(pud_test_node(5)), 4u);
}
