// Resolver: activates body subgoals and their DB candidates, detects conflict/unit
// goals, then deactivates parent candidates and goal expr.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/resolver.hpp"
#include "../../../core/hpp/infrastructure/rule_id_set.hpp"
#include "../../../core/hpp/interfaces/i_make_goal_lineage.hpp"
#include "../../../core/hpp/interfaces/i_make_resolution_lineage.hpp"
#include "../../../core/hpp/interfaces/i_goal_activator.hpp"
#include "../../../core/hpp/interfaces/i_goal_deactivator.hpp"
#include "../../../core/hpp/interfaces/i_get_rule.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_db_rule_ids.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "../../../core/hpp/interfaces/i_candidate_activator.hpp"
#include "../../../core/hpp/interfaces/i_candidate_deactivator.hpp"
#include "../../../core/hpp/interfaces/i_conflict_detector.hpp"
#include "../../../core/hpp/interfaces/i_detect_unit_goal.hpp"
#include "../../../core/hpp/interfaces/i_push_unit_goal.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockMakeGoalLineage : public i_make_goal_lineage {
    MOCK_METHOD((const goal_lineage*), make_goal_lineage,
        (const resolution_lineage*, subgoal_id), (override));
};

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id), (override));
};

struct MockGoalActivator : public i_goal_activator {
    MOCK_METHOD(void, activate, (const goal_lineage*), (override));
};

struct MockGoalDeactivator : public i_goal_deactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*), (override));
};

struct MockGetRule : public i_get_rule {
    MOCK_METHOD(const rule*, get, (rule_id), (const, override));
};

struct MockGetGoalDbRuleIds : public i_get_goal_db_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
};

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_id_set&, get, (const goal_lineage*), (const, override));
};

struct MockCandidateActivator : public i_candidate_activator {
    MOCK_METHOD(void, activate, (const resolution_lineage*), (override));
};

struct MockCandidateDeactivator : public i_candidate_deactivator {
    MOCK_METHOD(void, deactivate, (const resolution_lineage*), (override));
};

struct MockConflictDetector : public i_conflict_detector {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (const, override));
};

struct MockUnitGoalDetector : public i_detect_unit_goal {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (const, override));
};

struct MockPushUnitGoal : public i_push_unit_goal {
    MOCK_METHOD(void, push, (const goal_lineage*), (override));
};

struct ResolverTest : public ::testing::Test {
    MockMakeGoalLineage make_goal_lineage;
    MockMakeResolutionLineage make_resolution_lineage;
    MockGoalActivator goal_activator;
    MockGoalDeactivator goal_deactivator;
    MockGetRule get_rule;
    MockGetGoalDbRuleIds get_goal_db_rule_ids;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    MockCandidateActivator candidate_activator;
    MockCandidateDeactivator candidate_deactivator;
    MockConflictDetector conflict_detector;
    MockUnitGoalDetector unit_goal_detector;
    MockPushUnitGoal push_unit_goal;
    resolver res{
        make_goal_lineage,
        make_resolution_lineage,
        goal_activator,
        goal_deactivator,
        get_rule,
        get_goal_db_rule_ids,
        get_goal_candidate_rule_ids,
        candidate_activator,
        candidate_deactivator,
        conflict_detector,
        unit_goal_detector,
        push_unit_goal};

    static constexpr rule_id kRule = 0;
    static constexpr subgoal_id kBodyIdx = 0;

    expr parent_goal{expr::var{0}};
    expr body_goal{expr::var{1}};
    expr head{expr::var{10}};
    rule idx{&head, {&body_goal}};
    rule empty_body_rule{&head, {}};
    goal_lineage parent_gl{nullptr, 0};
    goal_lineage body_gl{nullptr, kBodyIdx};
    resolution_lineage rl{&parent_gl, kRule};
    resolution_lineage body_res{&body_gl, kRule};
    rule_id_set db_rules;
    rule_id_set parent_candidates;
};

TEST_F(ResolverTest, EmptyBodyDeactivatesParentOnly) {
    resolution_lineage empty_rl{&parent_gl, kRule};
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&empty_body_rule));
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&parent_gl)).WillOnce(ReturnRef(parent_candidates));
    EXPECT_CALL(goal_deactivator, deactivate(&parent_gl)).Times(1);
    EXPECT_TRUE(res.resolve(&empty_rl));
}

TEST_F(ResolverTest, ConflictOnBodyGoalReturnsFalse) {
    db_rules.insert(kRule);
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&idx));
    EXPECT_CALL(make_goal_lineage, make_goal_lineage(&rl, kBodyIdx)).WillOnce(Return(&body_gl));
    EXPECT_CALL(goal_activator, activate(&body_gl)).Times(1);
    EXPECT_CALL(get_goal_db_rule_ids, get(&body_gl)).WillOnce(ReturnRef(db_rules));
    EXPECT_CALL(make_resolution_lineage, make_resolution_lineage(&body_gl, kRule))
        .WillOnce(Return(&body_res));
    EXPECT_CALL(candidate_activator, activate(&body_res)).Times(1);
    EXPECT_CALL(conflict_detector, detect(&body_gl)).WillOnce(Return(true));
    EXPECT_CALL(goal_deactivator, deactivate).Times(0);
    EXPECT_FALSE(res.resolve(&rl));
}

TEST_F(ResolverTest, UnitBodyGoalIsPushed) {
    db_rules.insert(kRule);
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&idx));
    EXPECT_CALL(make_goal_lineage, make_goal_lineage(&rl, kBodyIdx)).WillOnce(Return(&body_gl));
    EXPECT_CALL(goal_activator, activate(&body_gl)).Times(1);
    EXPECT_CALL(get_goal_db_rule_ids, get(&body_gl)).WillOnce(ReturnRef(db_rules));
    EXPECT_CALL(make_resolution_lineage, make_resolution_lineage(&body_gl, kRule))
        .WillOnce(Return(&body_res));
    EXPECT_CALL(candidate_activator, activate(&body_res)).Times(1);
    EXPECT_CALL(conflict_detector, detect(&body_gl)).WillOnce(Return(false));
    EXPECT_CALL(unit_goal_detector, detect(&body_gl)).WillOnce(Return(true));
    EXPECT_CALL(push_unit_goal, push(&body_gl)).Times(1);
    EXPECT_CALL(get_goal_candidate_rule_ids, get(&parent_gl)).WillOnce(ReturnRef(parent_candidates));
    EXPECT_CALL(goal_deactivator, deactivate(&parent_gl)).Times(1);
    EXPECT_TRUE(res.resolve(&rl));
}
