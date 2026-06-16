// horizon_initial_goal_activator: delegates to initial_goal_activator, then assigns
// cached initial goal weight from i_get_initial_goal_weight.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "locator_fixture.hpp"
#include "infrastructure/horizon_initial_goal_activator.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "interfaces/i_set_goal_weight.hpp"
#include "interfaces/i_get_initial_goal_weight.hpp"
#include "interfaces/i_make_initial_goal_lineage.hpp"
#include "interfaces/i_get_initial_goal_expr.hpp"
#include "interfaces/i_set_goal_expr.hpp"
#include "interfaces/i_insert_goal_candidates.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "value_objects/expr.hpp"

using ::testing::NiceMock;
using ::testing::Return;

struct MockGetInitialGoalExpr : public i_get_initial_goal_expr {
    MOCK_METHOD(const expr*, get, (size_t), (const, override));
};

struct MockSetGoalExpr : public i_set_goal_expr {
    MOCK_METHOD(void, set, (const goal_lineage*, framed_expr), (override));
};

struct MockInsertGoalCandidates : public i_insert_goal_candidates {
    MOCK_METHOD(void, insert, (const goal_lineage*), (override));
};

struct MockInsertActiveGoal : public i_insert_active_goal {
    MOCK_METHOD(void, insert_active_goal, (const goal_lineage*), (override));
};

struct MockSetGoalWeight : public i_set_goal_weight {
    MOCK_METHOD(void, set, (const goal_lineage*, double), (override));
};

struct MockGetInitialGoalWeight : public i_get_initial_goal_weight {
    MOCK_METHOD(double, get, (), (const, override));
};

struct MockMakeInitialGoalLineage : public i_make_initial_goal_lineage {
    MOCK_METHOD((const goal_lineage*), make, (subgoal_id), (override));
};

struct MockInitialGoalActivator : public initial_goal_activator {
    explicit MockInitialGoalActivator(locator& loc) : initial_goal_activator(loc) {}
    MOCK_METHOD(void, activate_initial_goal, (subgoal_id), (override));
};

struct HorizonInitialGoalActivatorTest : public ::testing::Test {
    locator loc;
    NiceMock<MockGetInitialGoalExpr> get_initial_goal_expr;
    NiceMock<MockSetGoalExpr> set_goal_expr;
    NiceMock<MockInsertGoalCandidates> insert_goal_candidates;
    NiceMock<MockInsertActiveGoal> insert_active_goal;
    MockSetGoalWeight set_goal_weight;
    MockGetInitialGoalWeight get_initial_goal_weight;
    MockMakeInitialGoalLineage make_initial_goal_lineage;
    std::unique_ptr<MockInitialGoalActivator> mock_initial;
    std::unique_ptr<horizon_initial_goal_activator> activator;

    goal_lineage gl{nullptr, 0};
    static constexpr subgoal_id kIdx = 0;
    static constexpr double kInitialWeight = 0.5;

    void SetUp() override {
        loc.bind_as<i_get_initial_goal_expr>(get_initial_goal_expr);
        loc.bind_as<i_make_initial_goal_lineage>(make_initial_goal_lineage);
        loc.bind_as<i_set_goal_expr>(set_goal_expr);
        loc.bind_as<i_insert_goal_candidates>(insert_goal_candidates);
        loc.bind_as<i_insert_active_goal>(insert_active_goal);
        loc.bind_as<i_set_goal_weight>(set_goal_weight);
        loc.bind_as<i_get_initial_goal_weight>(get_initial_goal_weight);
        mock_initial = std::make_unique<MockInitialGoalActivator>(loc);
        loc.bind_as<initial_goal_activator>(*mock_initial);
        activator = std::make_unique<horizon_initial_goal_activator>(loc);
    }
};

TEST_F(HorizonInitialGoalActivatorTest, DelegatesThenSetsCachedInitialWeight) {
    testing::InSequence seq;
    EXPECT_CALL(*mock_initial, activate_initial_goal(kIdx)).Times(1);
    EXPECT_CALL(make_initial_goal_lineage, make(kIdx)).WillOnce(Return(&gl));
    EXPECT_CALL(get_initial_goal_weight, get()).WillOnce(Return(kInitialWeight));
    EXPECT_CALL(set_goal_weight, set(&gl, kInitialWeight)).Times(1);
    activator->activate_initial_goal(kIdx);
}
