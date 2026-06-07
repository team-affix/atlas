// subgoals_activator: body subgoal activation + goal_candidates per subgoal.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_make_goal_lineage.hpp"
#include "interfaces/i_goal_activator.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_activate_goal_candidates.hpp"

using ::testing::Return;

struct MockMakeGoalLineage : public i_make_goal_lineage {
    MOCK_METHOD((const goal_lineage*), make_goal_lineage,
        (const resolution_lineage*, subgoal_id), (override));
};

struct MockGoalActivator : public i_goal_activator {
    MOCK_METHOD(void, activate, (const goal_lineage*), (override));
};

struct MockGetRule : public i_get_rule {
    MOCK_METHOD(const rule*, get, (rule_id), (const, override));
};

struct MockActivateGoalCandidates : public i_activate_goal_candidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*), (override));
};

struct SubgoalsActivatorTest : public ::testing::Test {
    locator loc;
    MockMakeGoalLineage make_goal_lineage;
    MockGoalActivator goal_activator;
    MockGetRule get_rule;
    MockActivateGoalCandidates activate_goal_candidates;
    subgoals_activator activator;

    SubgoalsActivatorTest() : activator(init_activator()) {}

    subgoals_activator init_activator() {
        loc.bind_as<i_make_goal_lineage>(make_goal_lineage);
        loc.bind_as<i_goal_activator>(goal_activator);
        loc.bind_as<i_get_rule>(get_rule);
        loc.bind_as<i_activate_goal_candidates>(activate_goal_candidates);
        return subgoals_activator{loc};
    }

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
};

TEST_F(SubgoalsActivatorTest, EmptyBodyReturnsTrue) {
    resolution_lineage empty_rl{&parent_gl, kRule};
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&empty_body_rule));
    EXPECT_TRUE(activator.activate_subgoals(&empty_rl));
}

TEST_F(SubgoalsActivatorTest, ConflictOnBodyGoalReturnsFalse) {
    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&idx));
    EXPECT_CALL(make_goal_lineage, make_goal_lineage(&rl, kBodyIdx)).WillOnce(Return(&body_gl));
    EXPECT_CALL(goal_activator, activate(&body_gl)).Times(1);
    EXPECT_CALL(activate_goal_candidates, activate_goal_candidates(&body_gl)).WillOnce(Return(false));
    EXPECT_FALSE(activator.activate_subgoals(&rl));
}

TEST_F(SubgoalsActivatorTest, ActivatesEveryBodySubgoal) {
    expr body0{expr::var{1}};
    expr body1{expr::var{2}};
    rule two_body{&head, {&body0, &body1}};
    resolution_lineage rl_two{&parent_gl, kRule};
    goal_lineage body_gl0{&rl_two, 0};
    goal_lineage body_gl1{&rl_two, 1};

    EXPECT_CALL(get_rule, get(kRule)).WillOnce(Return(&two_body));
    EXPECT_CALL(make_goal_lineage, make_goal_lineage(&rl_two, 0)).WillOnce(Return(&body_gl0));
    EXPECT_CALL(make_goal_lineage, make_goal_lineage(&rl_two, 1)).WillOnce(Return(&body_gl1));
    EXPECT_CALL(goal_activator, activate(&body_gl0)).Times(1);
    EXPECT_CALL(goal_activator, activate(&body_gl1)).Times(1);
    EXPECT_CALL(activate_goal_candidates, activate_goal_candidates(&body_gl0)).WillOnce(Return(true));
    EXPECT_CALL(activate_goal_candidates, activate_goal_candidates(&body_gl1)).WillOnce(Return(true));
    EXPECT_TRUE(activator.activate_subgoals(&rl_two));
}
