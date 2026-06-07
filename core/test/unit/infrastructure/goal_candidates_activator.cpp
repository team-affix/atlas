// goal_candidates_activator: db-rule candidate activation, conflict check, unit push.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/goal_candidates_activator.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_get_goal_db_rule_ids.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_candidate_activator.hpp"
#include "interfaces/i_conflict_detector.hpp"
#include "interfaces/i_detect_unit_goal.hpp"
#include "interfaces/i_push_unit_goal.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

struct MockGetGoalDbRuleIds : public i_get_goal_db_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
};

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id), (override));
};

struct MockCandidateActivator : public i_candidate_activator {
    MOCK_METHOD(void, activate, (const resolution_lineage*), (override));
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

struct GoalCandidatesActivatorTest : public ::testing::Test {
    locator loc;
    MockGetGoalDbRuleIds get_goal_db_rule_ids;
    MockMakeResolutionLineage make_resolution_lineage;
    MockCandidateActivator candidate_activator;
    MockConflictDetector conflict_detector;
    MockUnitGoalDetector unit_goal_detector;
    MockPushUnitGoal push_unit_goal;
    goal_candidates_activator activator;

    GoalCandidatesActivatorTest() : activator(init_activator()) {}

    goal_candidates_activator init_activator() {
        loc.bind_as<i_get_goal_db_rule_ids>(get_goal_db_rule_ids);
        loc.bind_as<i_make_resolution_lineage>(make_resolution_lineage);
        loc.bind_as<i_candidate_activator>(candidate_activator);
        loc.bind_as<i_conflict_detector>(conflict_detector);
        loc.bind_as<i_detect_unit_goal>(unit_goal_detector);
        loc.bind_as<i_push_unit_goal>(push_unit_goal);
        return goal_candidates_activator{loc};
    }

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
