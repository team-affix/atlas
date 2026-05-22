#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/goal_activator.hpp"
#include "../../../core/hpp/interfaces/i_copier.hpp"
#include "../../../core/hpp/interfaces/i_goal_activator.hpp"
#include "../../../core/hpp/interfaces/i_activate_goal_expr.hpp"
#include "../../../core/hpp/interfaces/i_get_candidate_translation_map.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

struct MockActivateGoalExpr : public i_activate_goal_expr {
    MOCK_METHOD(void, activate, (const goal_lineage*, const expr*), (override));
};

struct MockGetCandidateTranslationMap : public i_get_candidate_translation_map {
    MOCK_METHOD(translation_map&, get, (const resolution_lineage*), (override));
};

struct MockCopier : public i_copier {
    MOCK_METHOD(const expr*, copy, (const expr*, translation_map&), (const, override));
};

struct GoalActivatorTest : public ::testing::Test {
    MockActivateGoalExpr age;
    MockGetCandidateTranslationMap gctm;
    MockCopier cp;

    expr parent_goal{expr::var{0}};
    expr child_goal{expr::var{1}};
    expr copied_goal{expr::var{99}};
    expr rule_head{expr::var{10}};
    rule parent_rule{&rule_head, {&child_goal}};

    resolution_lineage res{nullptr, &parent_rule};
    goal_lineage child_gl{&res, &child_goal};
    translation_map tm{{1, 2}};
};

TEST_F(GoalActivatorTest, ActivatePassesCopiedGoalExprToGoalExprActivator) {
    EXPECT_CALL(gctm, get(&res)).WillOnce(ReturnRef(tm));
    EXPECT_CALL(cp, copy(child_gl.idx, _)).WillOnce(Return(&copied_goal));
    EXPECT_CALL(age, activate(&child_gl, &copied_goal)).Times(1);

    goal_activator activator{age, gctm, cp};
    i_goal_activator& sut{activator};
    sut.activate(&child_gl);
}
