// goal_candidates_activator: db-rule candidate activation, conflict check, unit push.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/goal_candidates_activator.hpp"
#include "infrastructure/rule_id_set.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockGetGoalDbRuleIds {
    MOCK_METHOD(rule_id_set&, get, (const goal_lineage*));
};

struct MockMakeResolutionLineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id));
};

struct MockCandidateActivator {
    MOCK_METHOD(void, activate, (const resolution_lineage*));
};

struct MockConflictDetector {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (const));
};

struct MockUnitGoalDetector {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (const));
};

struct MockPushUnitGoal {
    MOCK_METHOD(void, push, (const goal_lineage*));
};

using test_goal_candidates_activator_t = goal_candidates_activator<
    MockGetGoalDbRuleIds, MockMakeResolutionLineage, MockCandidateActivator,
    MockConflictDetector, MockUnitGoalDetector, MockPushUnitGoal>;

struct GoalCandidatesActivatorTest : public ::testing::Test {
    MockGetGoalDbRuleIds get_goal_db_rule_ids;
    MockMakeResolutionLineage make_resolution_lineage;
    MockCandidateActivator candidate_activator;
    MockConflictDetector conflict_detector;
    MockUnitGoalDetector unit_goal_detector;
    MockPushUnitGoal push_unit_goal;
    test_goal_candidates_activator_t activator{get_goal_db_rule_ids, make_resolution_lineage,
                                          candidate_activator, conflict_detector,
                                          unit_goal_detector, push_unit_goal};

    goal_lineage gl{nullptr, 0};
    rule_id_set db_rules;
};

TEST_F(GoalCandidatesActivatorTest, ReturnsFalseOnConflict) {
    EXPECT_CALL(get_goal_db_rule_ids, get(&gl)).WillOnce(ReturnRef(db_rules));
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(true));
    EXPECT_CALL(push_unit_goal, push).Times(0);
    EXPECT_FALSE(activator.activate_goal_candidates(&gl));
}

TEST_F(GoalCandidatesActivatorTest, ActivatesDbRuleCandidates) {
    static constexpr rule_id kDbRule = 3;
    db_rules.insert(kDbRule);
    resolution_lineage db_rl{&gl, kDbRule};

    EXPECT_CALL(get_goal_db_rule_ids, get(&gl)).WillOnce(ReturnRef(db_rules));
    EXPECT_CALL(make_resolution_lineage, make_resolution_lineage(&gl, kDbRule))
        .WillOnce(Return(&db_rl));
    EXPECT_CALL(candidate_activator, activate(&db_rl)).Times(1);
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_CALL(unit_goal_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_TRUE(activator.activate_goal_candidates(&gl));
}

TEST_F(GoalCandidatesActivatorTest, PushesUnitGoalWhenDetected) {
    EXPECT_CALL(get_goal_db_rule_ids, get(&gl)).WillOnce(ReturnRef(db_rules));
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_CALL(unit_goal_detector, detect(&gl)).WillOnce(Return(true));
    EXPECT_CALL(push_unit_goal, push(&gl)).Times(1);
    EXPECT_TRUE(activator.activate_goal_candidates(&gl));
}
