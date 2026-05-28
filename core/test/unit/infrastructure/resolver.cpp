// Resolver: activates body subgoals and their DB candidates, detects conflict/unit
// goals, then deactivates parent candidates and goal expr. Conflict must abort before
// parent teardown; success must deactivate parent candidate set.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <type_traits>
#include "../../../core/hpp/infrastructure/resolver.hpp"
#include "../../../core/hpp/infrastructure/rule_set.hpp"
#include "../../../core/hpp/interfaces/i_make_goal_lineage.hpp"
#include "../../../core/hpp/interfaces/i_make_resolution_lineage.hpp"
#include "../../../core/hpp/interfaces/i_goal_activator.hpp"
#include "../../../core/hpp/interfaces/i_goal_deactivator.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_db_rules.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_candidate_rules.hpp"
#include "../../../core/hpp/interfaces/i_candidate_activator.hpp"
#include "../../../core/hpp/interfaces/i_candidate_deactivator.hpp"
#include "../../../core/hpp/interfaces/i_conflict_detector.hpp"
#include "../../../core/hpp/interfaces/i_detect_unit_goal.hpp"
#include "../../../core/hpp/interfaces/i_push_unit_goal.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockLineagePool
    : i_make_goal_lineage
    , i_make_resolution_lineage {
    MOCK_METHOD((const goal_lineage*), make, (const resolution_lineage*, subgoal_id), (override));
    MOCK_METHOD((const resolution_lineage*), make, (const goal_lineage*, rule_id), (override));
};

struct MockGoalActivator : public i_goal_activator {
    MOCK_METHOD(void, activate, (const goal_lineage*), (override));
};

struct MockGoalDeactivator : public i_goal_deactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*), (override));
};

struct MockGetGoalDbRules : public i_get_goal_db_rules {
    MOCK_METHOD(i_rule_set&, get, (const goal_lineage*), (override));
};

struct MockGetGoalCandidateRules : public i_get_goal_candidate_rules {
    MOCK_METHOD(i_rule_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_set&, get, (const goal_lineage*), (const, override));
};

struct MockCandidateActivator : public i_candidate_activator {
    MOCK_METHOD(void, activate, (const resolution_lineage*), (override));
};

struct MockCandidateDeactivator : public i_candidate_deactivator {
    MOCK_METHOD(void, deactivate, (const resolution_lineage*), (override));
};

struct MockConflictDetector : public i_conflict_detector {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (override));
};

struct MockUnitGoalDetector : public i_detect_unit_goal {
    MOCK_METHOD(bool, detect_goal, (const goal_lineage*), (const));
    bool detect(const goal_lineage* gl) const override { return detect_goal(gl); }
};

struct MockPushUnitGoal : public i_push_unit_goal {
    MOCK_METHOD(void, push, (const goal_lineage*), (override));
};

struct ResolverTest : public ::testing::Test {
    MockLineagePool lp;
    MockGoalActivator goal_activator;
    MockGoalDeactivator goal_deactivator;
    MockGetGoalDbRules ggdr;
    MockGetGoalCandidateRules ggcr;
    MockCandidateActivator candidate_activator;
    MockCandidateDeactivator candidate_deactivator;
    MockConflictDetector conflict_detector;
    MockUnitGoalDetector unit_goal_detector;
    MockPushUnitGoal push_unit_goal;
    resolver res{
        lp,
        lp,
        goal_activator,
        goal_deactivator,
        ggdr,
        ggcr,
        candidate_activator,
        candidate_deactivator,
        conflict_detector,
        unit_goal_detector,
        push_unit_goal};

    expr parent_goal{expr::var{0}};
    expr body_goal{expr::var{1}};
    expr head{expr::var{10}};
    expr db_head{expr::var{20}};
    rule idx{&head, {&body_goal}};
    rule db_rule{&db_head, {}};
    goal_lineage parent_gl{nullptr, &parent_goal};
    goal_lineage body_gl{nullptr, &body_goal};
    resolution_lineage rl{&parent_gl, &idx};
    resolution_lineage body_res{&body_gl, &db_rule};
    rule_set db_rules;
    rule_set parent_candidates;
};

TEST_F(ResolverTest, EmptyBodyDeactivatesParentOnly) {
    rule empty_body_rule{&head, {}};
    resolution_lineage empty_rl{&parent_gl, &empty_body_rule};
    resolution_lineage empty_res{&parent_gl, &empty_body_rule};
    parent_candidates.insert(&empty_body_rule);
    EXPECT_CALL(ggcr, get(&parent_gl)).WillOnce(ReturnRef(parent_candidates));
    EXPECT_CALL(lp, make(&parent_gl, &empty_body_rule)).WillOnce(Return(&empty_res));
    EXPECT_CALL(candidate_deactivator, deactivate(&empty_res)).Times(1);
    EXPECT_CALL(goal_deactivator, deactivate(&parent_gl)).Times(1);
    EXPECT_TRUE(res.resolve(&empty_rl));
}

TEST_F(ResolverTest, ConflictOnBodyGoalReturnsFalse) {
    db_rules.insert(&db_rule);
    EXPECT_CALL(lp, make(&rl, &body_goal)).WillOnce(Return(&body_gl));
    EXPECT_CALL(goal_activator, activate(&body_gl)).Times(1);
    EXPECT_CALL(ggdr, get(&body_gl)).WillOnce(ReturnRef(db_rules));
    EXPECT_CALL(lp, make(&body_gl, &db_rule)).WillOnce(Return(&body_res));
    EXPECT_CALL(candidate_activator, activate(&body_res)).Times(1);
    EXPECT_CALL(conflict_detector, detect(&body_gl)).WillOnce(Return(true));
    EXPECT_CALL(goal_deactivator, deactivate).Times(0);
    EXPECT_FALSE(res.resolve(&rl));
}

TEST_F(ResolverTest, UnitBodyGoalIsPushed) {
    db_rules.insert(&db_rule);
    EXPECT_CALL(lp, make(&rl, &body_goal)).WillOnce(Return(&body_gl));
    EXPECT_CALL(goal_activator, activate(&body_gl)).Times(1);
    EXPECT_CALL(ggdr, get(&body_gl)).WillOnce(ReturnRef(db_rules));
    EXPECT_CALL(lp, make(&body_gl, &db_rule)).WillOnce(Return(&body_res));
    EXPECT_CALL(candidate_activator, activate(&body_res)).Times(1);
    EXPECT_CALL(conflict_detector, detect(&body_gl)).WillOnce(Return(false));
    EXPECT_CALL(unit_goal_detector, detect_goal(&body_gl)).WillOnce(Return(true));
    EXPECT_CALL(push_unit_goal, push(&body_gl)).Times(1);
    EXPECT_CALL(ggcr, get(&parent_gl)).WillOnce(ReturnRef(parent_candidates));
    EXPECT_CALL(goal_deactivator, deactivate(&parent_gl)).Times(1);
    EXPECT_TRUE(res.resolve(&rl));
}
