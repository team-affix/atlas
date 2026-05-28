// Solver coroutine: wraps sim set_up/run/tear_down and yields each termination. The
// first resume must reflect run_sim output; set_up precedes run and tear_down follows.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/solver.hpp"
#include "interfaces/i_set_up_sim.hpp"
#include "interfaces/i_tear_down_sim.hpp"
#include "interfaces/i_run_sim.hpp"
#include "interfaces/i_learn_avoidance.hpp"
#include "interfaces/i_elimination_router.hpp"
#include "interfaces/i_get_decision_count.hpp"
#include "interfaces/i_derive_decision_lemma.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"

using ::testing::Return;
using ::testing::_;

struct MockSetUpSim : public i_set_up_sim {
    MOCK_METHOD(void, set_up, (), (override));
};

struct MockTearDownSim : public i_tear_down_sim {
    MOCK_METHOD(void, tear_down, (), (override));
};

struct MockRunSim : public i_run_sim {
    MOCK_METHOD(sim_termination, run, (), (override));
};

struct MockLearnAvoidance : public i_learn_avoidance {
    MOCK_METHOD(std::optional<const resolution_lineage*>, learn, (const lemma&), (override));
};

struct MockEliminationRouter : public i_elimination_router {
    MOCK_METHOD(elimination_result, route, (const resolution_lineage*), (override));
};

struct MockGetDecisionCount : public i_get_decision_count {
    MOCK_METHOD(size_t, count, (), (const, override));
};

struct MockDeriveDecisionLemma : public i_derive_decision_lemma {
    MOCK_METHOD(lemma, derive, (), (const, override));
};

struct SolverTest : public ::testing::Test {
    MockSetUpSim set_up_sim;
    MockTearDownSim tear_down_sim;
    MockRunSim run_sim;
    MockGetDecisionCount get_decision_count;
    MockDeriveDecisionLemma derive_decision_lemma;
    MockLearnAvoidance learn_avoidance;
    MockEliminationRouter router;
    solver s{
        set_up_sim,
        tear_down_sim,
        run_sim,
        get_decision_count,
        derive_decision_lemma,
        learn_avoidance,
        router};
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
    const lemma empty_lemma{{}};
    testing::InSequence seq;
    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(get_decision_count, count()).WillOnce(Return(0));
    EXPECT_CALL(derive_decision_lemma, derive()).WillOnce(Return(empty_lemma));
    EXPECT_CALL(learn_avoidance, learn(testing::_)).WillOnce(Return(std::nullopt));
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

TEST_F(SolverTest, RunsSecondIterationWhileDecisionsRemain) {
    const lemma empty_lemma{{}};
    EXPECT_CALL(set_up_sim, set_up()).Times(2);
    EXPECT_CALL(run_sim, run())
        .WillOnce(Return(sim_termination::conflicted))
        .WillOnce(Return(sim_termination::solved));
    EXPECT_CALL(get_decision_count, count())
        .WillOnce(Return(1))
        .WillOnce(Return(0));
    EXPECT_CALL(derive_decision_lemma, derive())
        .WillRepeatedly(Return(empty_lemma));
    EXPECT_CALL(learn_avoidance, learn(testing::_))
        .WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(tear_down_sim, tear_down()).Times(2);

    auto sm = s.solve();
    ASSERT_EQ(sm.resume(), sim_termination::conflicted);
    ASSERT_EQ(sm.resume(), sim_termination::solved);
    EXPECT_FALSE(sm.resume().has_value());
}

TEST_F(SolverTest, YieldPropagatesDepthExceededTermination) {
    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::depth_exceeded));
    auto sm = s.solve();
    auto term = sm.resume();
    ASSERT_TRUE(term.has_value());
    EXPECT_EQ(*term, sim_termination::depth_exceeded);
}

TEST_F(SolverTest, RoutesEliminationWhenLearnReturnsLineage) {
    goal_lineage gl{nullptr, 0};
    resolution_lineage elim{&gl, 1};
    const lemma empty_lemma{{}};

    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(get_decision_count, count()).WillOnce(Return(0));
    EXPECT_CALL(derive_decision_lemma, derive()).WillOnce(Return(empty_lemma));
    EXPECT_CALL(learn_avoidance, learn(testing::_)).WillOnce(Return(&elim));
    EXPECT_CALL(router, route(&elim)).Times(1);
    EXPECT_CALL(tear_down_sim, tear_down()).Times(1);

    auto sm = s.solve();
    ASSERT_EQ(sm.resume(), sim_termination::conflicted);
    EXPECT_FALSE(sm.resume().has_value());
}
