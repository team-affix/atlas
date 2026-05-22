#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/goal_candidate_activator_visitor.hpp"
#include "../../../core/hpp/infrastructure/lineage_pool.hpp"
#include "../../../core/hpp/infrastructure/copier.hpp"
#include "../../../core/hpp/infrastructure/var_sequencer.hpp"
#include "../../../core/hpp/infrastructure/expr_pool.hpp"
#include "../../../core/hpp/infrastructure/bind_map.hpp"
#include "../../../core/hpp/infrastructure/bind_map_factory.hpp"
#include "../../../core/hpp/infrastructure/overlay_bind_map_factory.hpp"
#include "../../../core/hpp/infrastructure/unifier_factory.hpp"
#include "../../../core/hpp/infrastructure/mhu_elimination_generator.hpp"
#include "../../../core/hpp/utility/trail.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_expr.hpp"
#include "../../../core/hpp/interfaces/i_activate_candidate_translation_map.hpp"
#include "../../../core/hpp/interfaces/i_candidate_activator.hpp"
#include "../../../core/hpp/interfaces/i_elimination_backlog.hpp"

using ::testing::_;
using ::testing::Return;

struct MockGetGoalExpr : public i_get_goal_expr {
    MOCK_METHOD(const expr*, get, (const goal_lineage*), (const, override));
};

struct MockActivateCandidateTranslationMap : public i_activate_candidate_translation_map {
    MOCK_METHOD(void, activate, (const resolution_lineage*, translation_map), (override));
};

struct MockCandidateActivator : public i_candidate_activator {
    MOCK_METHOD(void, activate, (const resolution_lineage*), (override));
};

struct MockEliminationBacklog : public i_elimination_backlog {
    MOCK_METHOD(void, insert, (const resolution_lineage*), (override));
    MOCK_METHOD(bool, contains, (const resolution_lineage*), (override));
    MOCK_METHOD(void, constrain, (const resolution_lineage*), (override));
};

struct GoalCandidateActivatorVisitorIntegrationTest : public ::testing::Test {
    trail t;
    expr_pool pool{t};
    var_sequencer vs{t};
    copier cp{vs, pool};
    bind_map common;
    bind_map_factory bmf;
    overlay_bind_map_factory obmf;
    unifier_factory uf;
    lineage_pool lp;
    mhu_elimination_generator mhu{common, pool};

    MockGetGoalExpr gge;
    MockActivateCandidateTranslationMap actm;
    MockCandidateActivator ca;
    MockEliminationBacklog eb;

    expr goal_var{expr::var{0}};
    expr head_var{expr::var{0}};
    rule matching{&head_var, {}};

    goal_lineage* gl = nullptr;

    void SetUp() override {
        gl = const_cast<goal_lineage*>(lp.goal(nullptr, &goal_var));
        ON_CALL(eb, contains(_)).WillByDefault(Return(false));
    }
};

TEST_F(GoalCandidateActivatorVisitorIntegrationTest,
    SuccessfulUnifyActivatesCandidateAndTranslationMap) {
    const resolution_lineage* rl = lp.resolution(gl, &matching);

    EXPECT_CALL(gge, get(gl)).WillRepeatedly(Return(&goal_var));
    EXPECT_CALL(ca, activate(rl)).Times(1);
    EXPECT_CALL(actm, activate(rl, _)).Times(1);

    goal_candidate_activator_visitor visitor{
        gl, cp, common, bmf, obmf, uf, actm, mhu, ca, eb, lp, gge};

    visitor.visit(&matching);
}

TEST_F(GoalCandidateActivatorVisitorIntegrationTest,
    FailedUnifyDoesNotActivateCandidate) {
    expr goal_f{expr::functor{"f", {}}};
    expr head_g{expr::functor{"g", {}}};
    rule non_matching{&head_g, {}};

    auto* gl_f = const_cast<goal_lineage*>(lp.goal(nullptr, &goal_f));
    const resolution_lineage* rl = lp.resolution(gl_f, &non_matching);

    EXPECT_CALL(gge, get(gl_f)).WillRepeatedly(Return(&goal_f));
    EXPECT_CALL(ca, activate(rl)).Times(0);
    EXPECT_CALL(actm, activate).Times(0);

    goal_candidate_activator_visitor visitor{
        gl_f, cp, common, bmf, obmf, uf, actm, mhu, ca, eb, lp, gge};

    visitor.visit(&non_matching);
}

TEST_F(GoalCandidateActivatorVisitorIntegrationTest,
    SkippedWhenEliminatedDoesNotActivate) {
    expr goal_f{expr::functor{"f", {}}};
    expr head_f{expr::functor{"f", {}}};
    rule rule_f{&head_f, {}};

    auto* gl_f = const_cast<goal_lineage*>(lp.goal(nullptr, &goal_f));
    const resolution_lineage* rl = lp.resolution(gl_f, &rule_f);

    EXPECT_CALL(eb, contains(rl)).WillOnce(Return(true));
    EXPECT_CALL(ca, activate).Times(0);
    EXPECT_CALL(actm, activate).Times(0);

    goal_candidate_activator_visitor visitor{
        gl_f, cp, common, bmf, obmf, uf, actm, mhu, ca, eb, lp, gge};

    visitor.visit(&rule_f);
}
