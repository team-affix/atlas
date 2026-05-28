// goal_activator copies the parent rule's body subgoal through the candidate translation
// map and registers it via i_set_goal_expr.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/goal_activator.hpp"
#include "../../../core/hpp/interfaces/i_copier.hpp"
#include "../../../core/hpp/interfaces/i_set_goal_expr.hpp"
#include "../../../core/hpp/interfaces/i_get_candidate_translation_map.hpp"
#include "../../../core/hpp/interfaces/i_get_resolution_rule.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

struct MockSetGoalExpr : public i_set_goal_expr {
    MOCK_METHOD(void, set, (const goal_lineage*, const expr*), (override));
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

    MockSetGoalExpr set_goal_expr;
    MockGetCandidateTranslationMap get_candidate_translation_map;
    MockGetResolutionRule get_resolution_rule;
    MockCopier copier;
    goal_activator activator{
        set_goal_expr,
        get_candidate_translation_map,
        get_resolution_rule,
        copier};

    expr child_goal{expr::var{1}};
    expr copied_goal{expr::var{99}};
    expr rule_head{expr::var{10}};
    rule parent_rule{&rule_head, {&child_goal}};

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage res{&parent_gl, kRule};
    goal_lineage child_gl{&res, kBodyIdx};
    translation_map tm{{1, 2}};
};

TEST_F(GoalActivatorTest, ActivateCopiesBodySubgoalThroughTranslationMap) {
    EXPECT_CALL(get_resolution_rule, get(&res)).WillOnce(Return(&parent_rule));
    EXPECT_CALL(get_candidate_translation_map, get(&res)).WillOnce(ReturnRef(tm));
    EXPECT_CALL(copier, copy(&child_goal, _)).WillOnce(Return(&copied_goal));
    EXPECT_CALL(set_goal_expr, set(&child_gl, &copied_goal)).Times(1);

    activator.activate(&child_gl);
}
