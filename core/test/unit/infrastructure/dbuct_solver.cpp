// dbuct_solver: delayed-backtracking solve coroutine contracts with mocked collaborators.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/dbuct_solver.hpp"
#include "value_objects/elimination_result.hpp"
#include "value_objects/sim_termination.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockActivateInitialGoalsOnce {
    MOCK_METHOD(bool, activate_initial_goals_and_candidates, ());
};

struct MockRunSim {
    MOCK_METHOD(sim_termination, run, ());
};

struct MockGetDecisionCount {
    MOCK_METHOD(size_t, count, (), (const));
};

struct MockTerminate {
    MOCK_METHOD((std::vector<const resolution_lineage*>), terminate, ());
};

struct MockCheckIsAtRoot {
    MOCK_METHOD(bool, at_root, (), (const));
};

struct MockLearnAvoidance {
    MOCK_METHOD(void, learn, ());
};

struct MockRemoveMhuHead {
    MOCK_METHOD(void, remove_head, (const resolution_lineage*));
};

struct MockEliminationRouter {
    MOCK_METHOD(elimination_result, route, (const resolution_lineage*));
};

struct MockConflictDetector {
    MOCK_METHOD(bool, detect, (const goal_lineage*));
};

struct MockUnitGoalDetector {
    MOCK_METHOD(bool, detect, (const goal_lineage*));
};

struct MockPushUnitGoal {
    MOCK_METHOD(void, push, (const goal_lineage*));
};

using test_dbuct_solver_t = dbuct_solver<
    MockActivateInitialGoalsOnce, MockRunSim, MockGetDecisionCount, MockTerminate,
    MockCheckIsAtRoot, MockLearnAvoidance, MockRemoveMhuHead, MockEliminationRouter,
    MockConflictDetector, MockUnitGoalDetector, MockPushUnitGoal>;

struct DbuctSolverTest : public ::testing::Test {
    NiceMock<MockActivateInitialGoalsOnce> activate;
    NiceMock<MockRunSim> run_sim;
    NiceMock<MockGetDecisionCount> get_decision_count;
    NiceMock<MockTerminate> terminate;
    NiceMock<MockCheckIsAtRoot> check_is_at_root;
    NiceMock<MockLearnAvoidance> learn;
    NiceMock<MockRemoveMhuHead> remove_mhu_head;
    NiceMock<MockEliminationRouter> router;
    NiceMock<MockConflictDetector> conflict_detector;
    NiceMock<MockUnitGoalDetector> unit_goal_detector;
    NiceMock<MockPushUnitGoal> push_unit_goal;

    test_dbuct_solver_t solver{
        activate, run_sim, get_decision_count, terminate, check_is_at_root,
        learn, remove_mhu_head, router, conflict_detector, unit_goal_detector,
        push_unit_goal};

    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, 0};
};

TEST_F(DbuctSolverTest, ActivationFailureYieldsConflicted) {
    EXPECT_CALL(activate, activate_initial_goals_and_candidates()).WillOnce(Return(false));

    auto sm = solver.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::conflicted);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(DbuctSolverTest, SuccessfulActivationForwardsRunYieldThenRefutesWithNoDecisions) {
    EXPECT_CALL(activate, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::solved));
    EXPECT_CALL(get_decision_count, count()).WillOnce(Return(0));

    auto sm = solver.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(DbuctSolverTest, AfterYieldWithDecisionsLearnsTerminatesAndRoutes) {
    EXPECT_CALL(activate, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(run_sim, run())
        .WillOnce(Return(sim_termination::conflicted))
        .WillOnce(Return(sim_termination::solved));
    EXPECT_CALL(get_decision_count, count())
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    EXPECT_CALL(learn, learn()).Times(1);
    EXPECT_CALL(terminate, terminate())
        .WillOnce(Return(std::vector<const resolution_lineage*>{&rl}));
    EXPECT_CALL(remove_mhu_head, remove_head(&rl)).Times(1);
    EXPECT_CALL(router, route(&rl)).WillOnce(Return(elimination_result::eliminated));
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_CALL(unit_goal_detector, detect(&gl)).WillOnce(Return(false));

    auto sm = solver.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::conflicted);
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(DbuctSolverTest, CascadeStopsWhenRoutingRealizesNoConflict) {
    EXPECT_CALL(activate, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(run_sim, run())
        .WillOnce(Return(sim_termination::conflicted))
        .WillOnce(Return(sim_termination::solved));
    EXPECT_CALL(get_decision_count, count())
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    EXPECT_CALL(learn, learn()).Times(1);
    EXPECT_CALL(terminate, terminate())
        .WillOnce(Return(std::vector<const resolution_lineage*>{&rl}));
    EXPECT_CALL(remove_mhu_head, remove_head(&rl)).Times(1);
    EXPECT_CALL(router, route(&rl)).WillOnce(Return(elimination_result::already_deactivated));

    auto sm = solver.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    sm.consume_yield();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(DbuctSolverTest, ConflictAtRootRefutes) {
    EXPECT_CALL(activate, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(get_decision_count, count()).WillOnce(Return(1));
    EXPECT_CALL(learn, learn()).Times(1);
    EXPECT_CALL(terminate, terminate())
        .WillOnce(Return(std::vector<const resolution_lineage*>{&rl}));
    EXPECT_CALL(remove_mhu_head, remove_head(&rl)).Times(1);
    EXPECT_CALL(router, route(&rl)).WillOnce(Return(elimination_result::eliminated));
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(true));
    EXPECT_CALL(check_is_at_root, at_root()).WillOnce(Return(true));

    auto sm = solver.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::conflicted);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(DbuctSolverTest, UnitGoalIsPushedWhenRoutingCollapsesToUnit) {
    EXPECT_CALL(activate, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(run_sim, run())
        .WillOnce(Return(sim_termination::conflicted))
        .WillOnce(Return(sim_termination::solved));
    EXPECT_CALL(get_decision_count, count())
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    EXPECT_CALL(learn, learn()).Times(1);
    EXPECT_CALL(terminate, terminate())
        .WillOnce(Return(std::vector<const resolution_lineage*>{&rl}));
    EXPECT_CALL(remove_mhu_head, remove_head(&rl)).Times(1);
    EXPECT_CALL(router, route(&rl)).WillOnce(Return(elimination_result::eliminated));
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_CALL(unit_goal_detector, detect(&gl)).WillOnce(Return(true));
    EXPECT_CALL(push_unit_goal, push(&gl)).Times(1);

    auto sm = solver.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    sm.consume_yield();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    sm.consume_yield();
    sm.resume();
    EXPECT_TRUE(sm.done());
}

}  // namespace
