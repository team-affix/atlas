// horizon_goal_activator: delegates to goal_activator, then assigns parent_w/g child weight.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "locator_fixture.hpp"
#include "infrastructure/horizon_goal_activator.hpp"
#include "infrastructure/goal_activator.hpp"
#include "interfaces/i_set_goal_weight.hpp"
#include "interfaces/i_get_goal_weight.hpp"
#include "interfaces/i_get_rule.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/expr.hpp"
#include "interfaces/i_set_goal_expr.hpp"
#include "interfaces/i_insert_goal_candidates.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_get_candidate_translation_map.hpp"
#include "interfaces/i_get_resolution_rule.hpp"
#include "interfaces/i_copier.hpp"
#include "value_objects/translation_map.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

struct MockSetGoalExpr : public i_set_goal_expr {
    MOCK_METHOD(void, set, (const goal_lineage*, const expr*), (override));
};

struct MockInsertGoalCandidates : public i_insert_goal_candidates {
    MOCK_METHOD(void, insert, (const goal_lineage*), (override));
};

struct MockInsertActiveGoal : public i_insert_active_goal {
    MOCK_METHOD(void, insert_active_goal, (const goal_lineage*), (override));
};

struct MockGetCandidateTranslationMap : public i_get_candidate_translation_map {
    MOCK_METHOD(translation_map&, get, (const resolution_lineage*), (override));
};

struct MockGetResolutionRule : public i_get_resolution_rule {
    MOCK_METHOD(const rule*, get, (const resolution_lineage*), (const, override));
};

struct MockCopier : public i_copier {
    MOCK_METHOD(const expr*, copy, (const expr*, translation_map&), (const, override));
};

struct MockSetGoalWeight : public i_set_goal_weight {
    MOCK_METHOD(void, set, (const goal_lineage*, double), (override));
};

struct MockGetGoalWeight : public i_get_goal_weight {
    MOCK_METHOD(double, get, (const goal_lineage*), (const, override));
};

struct MockGetRule : public i_get_rule {
    MOCK_METHOD(const rule*, get, (rule_id), (const, override));
};

struct MockDelegateGoalActivator : public goal_activator {
    explicit MockDelegateGoalActivator(locator& loc) : goal_activator(loc) {}
    MOCK_METHOD(void, activate, (const goal_lineage*), (override));
};

struct HorizonGoalActivatorTest : public ::testing::Test {
    locator loc;
    NiceMock<MockSetGoalExpr> set_goal_expr;
    NiceMock<MockInsertGoalCandidates> insert_goal_candidates;
    NiceMock<MockInsertActiveGoal> insert_active_goal;
    NiceMock<MockGetCandidateTranslationMap> get_candidate_translation_map;
    NiceMock<MockGetResolutionRule> get_resolution_rule;
    NiceMock<MockCopier> copier;
    MockSetGoalWeight set_goal_weight;
    MockGetGoalWeight get_goal_weight;
    MockGetRule get_rule;
    std::unique_ptr<MockDelegateGoalActivator> mock_goal_activator;
    std::unique_ptr<horizon_goal_activator> activator;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 1};
    goal_lineage child_gl{&rl, 0};
    translation_map tm;

    expr head{expr::var{0}};
    expr body0{expr::var{1}};
    expr body1{expr::var{2}};
    rule two_body_rule{&head, {&body0, &body1}};

    static constexpr double kParentWeight = 1.0;
    static constexpr double kExpectedChildWeight = 0.5;

    void SetUp() override {
        ON_CALL(get_candidate_translation_map, get(&rl)).WillByDefault(ReturnRef(tm));
        loc.bind_as<i_set_goal_expr>(set_goal_expr);
        loc.bind_as<i_insert_goal_candidates>(insert_goal_candidates);
        loc.bind_as<i_insert_active_goal>(insert_active_goal);
        loc.bind_as<i_get_candidate_translation_map>(get_candidate_translation_map);
        loc.bind_as<i_get_resolution_rule>(get_resolution_rule);
        loc.bind_as<i_copier>(copier);
        loc.bind_as<i_set_goal_weight>(set_goal_weight);
        loc.bind_as<i_get_goal_weight>(get_goal_weight);
        loc.bind_as<i_get_rule>(get_rule);
        mock_goal_activator = std::make_unique<MockDelegateGoalActivator>(loc);
        loc.bind_as<goal_activator>(*mock_goal_activator);
        activator = std::make_unique<horizon_goal_activator>(loc);
    }
};

TEST_F(HorizonGoalActivatorTest, DelegatesThenSetsParentWeightDividedByBodySize) {
    testing::InSequence seq;
    EXPECT_CALL(*mock_goal_activator, activate(&child_gl)).Times(1);
    EXPECT_CALL(get_rule, get(rl.idx)).WillOnce(Return(&two_body_rule));
    EXPECT_CALL(get_goal_weight, get(&parent_gl)).WillOnce(Return(kParentWeight));
    EXPECT_CALL(set_goal_weight, set(&child_gl, kExpectedChildWeight)).Times(1);
    activator->activate(&child_gl);
}
