// goal_activator copies the parent rule's body subgoal through the candidate translation
// map and registers it via i_set_goal_expr.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include "infrastructure/goal_activator.hpp"
#include "interfaces/i_copier.hpp"
#include "interfaces/i_set_goal_expr.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_get_candidate_translation_map.hpp"
#include "interfaces/i_get_resolution_rule.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

struct MockSetGoalExpr : public i_set_goal_expr {
    MOCK_METHOD(void, set, (const goal_lineage*, const expr*), (override));
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

struct GoalActivatorTest : public ::testing::Test {
    static constexpr rule_id kRule = 0;
    static constexpr subgoal_id kBodyIdx = 0;

    locator loc;
    MockSetGoalExpr set_goal_expr;
    MockInsertActiveGoal insert_active_goal;
    MockGetCandidateTranslationMap get_candidate_translation_map;
    MockGetResolutionRule get_resolution_rule;
    MockCopier copier;
    goal_activator activator;

    GoalActivatorTest() : activator(init_activator()) {}

    goal_activator init_activator() {
        loc.bind_as<i_set_goal_expr>(set_goal_expr);
        loc.bind_as<i_insert_active_goal>(insert_active_goal);
        loc.bind_as<i_get_candidate_translation_map>(get_candidate_translation_map);
        loc.bind_as<i_get_resolution_rule>(get_resolution_rule);
        loc.bind_as<i_copier>(copier);
        return goal_activator{loc};
    }

    expr child_goal{expr::var{1}};
    expr copied_goal{expr::var{99}};
    expr rule_head{expr::var{10}};
    rule parent_rule{&rule_head, {&child_goal}};

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage res{&parent_gl, kRule};
    goal_lineage child_gl{&res, kBodyIdx};
    translation_map tm{{1, 2}};
};

TEST_F(GoalActivatorTest, ActivateUsesBodyIndexForSubgoalExpr) {
    expr second_body{expr::var{2}};
    expr copied_second{expr::var{88}};
    rule two_body{&rule_head, {&child_goal, &second_body}};
    goal_lineage second_gl{&res, 1};

    EXPECT_CALL(get_resolution_rule, get(&res)).WillOnce(Return(&two_body));
    EXPECT_CALL(get_candidate_translation_map, get(&res)).WillOnce(ReturnRef(tm));
    EXPECT_CALL(copier, copy(&second_body, _)).WillOnce(Return(&copied_second));
    EXPECT_CALL(set_goal_expr, set(&second_gl, &copied_second)).Times(1);
    EXPECT_CALL(insert_active_goal, insert_active_goal(&second_gl)).Times(1);

    activator.activate(&second_gl);
}

TEST_F(GoalActivatorTest, ActivateCopiesBodySubgoalThroughTranslationMap) {
    EXPECT_CALL(get_resolution_rule, get(&res)).WillOnce(Return(&parent_rule));
    EXPECT_CALL(get_candidate_translation_map, get(&res)).WillOnce(ReturnRef(tm));
    EXPECT_CALL(copier, copy(&child_goal, _)).WillOnce(Return(&copied_goal));
    EXPECT_CALL(set_goal_expr, set(&child_gl, &copied_goal)).Times(1);
    EXPECT_CALL(insert_active_goal, insert_active_goal(&child_gl)).Times(1);

    activator.activate(&child_gl);
}
