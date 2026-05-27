// Solver coroutine: wraps sim set_up/run/tear_down and yields each termination. The
// first resume must reflect run_sim output; set_up precedes run and tear_down follows.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/solver.hpp"
#include "../../../core/hpp/interfaces/i_set_up_sim.hpp"
#include "../../../core/hpp/interfaces/i_tear_down_sim.hpp"
#include "../../../core/hpp/interfaces/i_run_sim.hpp"
#include "../../../core/hpp/interfaces/i_cdcl_elimination_generator.hpp"
#include "../../../core/hpp/interfaces/i_elimination_router.hpp"
#include "../../../core/hpp/value_objects/sim_termination.hpp"

using ::testing::Return;

struct MockSetUpSim : public i_set_up_sim {
    MOCK_METHOD(void, set_up, (), (override));
};

struct MockTearDownSim : public i_tear_down_sim {
    MOCK_METHOD(void, tear_down, (), (override));
};

struct MockRunSim : public i_run_sim {
    MOCK_METHOD(sim_termination, run, (), (override));
};

struct MockCdclEliminationGenerator : public i_cdcl_elimination_generator {
    MOCK_METHOD(state_machine<const resolution_lineage*>, constrain, (const resolution_lineage*), (override));
    MOCK_METHOD(const resolution_lineage*, learn, (const lemma&), (override));
};

struct MockEliminationRouter : public i_elimination_router {
    MOCK_METHOD(elimination_result, route, (const resolution_lineage*), (override));
};

struct SolverTest : public ::testing::Test {
    MockSetUpSim set_up_sim;
    MockTearDownSim tear_down_sim;
    MockRunSim run_sim;
    MockCdclEliminationGenerator cdcl;
    MockEliminationRouter router;
    solver s{set_up_sim, tear_down_sim, run_sim, cdcl, router};
};

TEST_F(SolverTest, FirstYieldRunsSetUpThenRunBeforeTearDown) {
    testing::InSequence seq;
    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::solved));
    auto sm = s.solve();
    auto term = sm.resume();
    ASSERT_TRUE(term.has_value());
    EXPECT_EQ(*term, sim_termination::solved);
}

TEST_F(SolverTest, TearDownRunsOnResumeAfterYield) {
    EXPECT_CALL(run_sim, run()).WillRepeatedly(Return(sim_termination::conflicted));
    EXPECT_CALL(tear_down_sim, tear_down()).Times(1);
    auto sm = s.solve();
    ASSERT_TRUE(sm.resume().has_value());
    sm.resume();
}

TEST_F(SolverTest, YieldPropagatesConflictTermination) {
    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::conflicted));
    auto sm = s.solve();
    auto term = sm.resume();
    ASSERT_TRUE(term.has_value());
    EXPECT_EQ(*term, sim_termination::conflicted);
}
