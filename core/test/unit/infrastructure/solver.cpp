// solver_t coroutine: set_up, run (yield), derive, pin lemma resolutions, tear_down, learn/route.
// Post-yield work runs on the second resume(); learn runs after tear_down (trail pop).

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/solver.hpp"
#include "value_objects/elimination_result.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"

using ::testing::Return;
using ::testing::_;

namespace {

struct MockSetUpSim {
    MOCK_METHOD(void, set_up, ());
};

struct MockTearDownSim {
    MOCK_METHOD(void, tear_down, ());
};

struct MockRunSim {
    MOCK_METHOD(sim_termination, run, ());
};

struct MockLearnAvoidance {
    MOCK_METHOD(std::optional<const resolution_lineage*>, learn, (const lemma&));
};

struct MockEliminationRouter {
    MOCK_METHOD(elimination_result, route, (const resolution_lineage*));
};

struct MockGetDecisionCount {
    MOCK_METHOD(size_t, count, (), (const));
};

struct MockDeriveDecisionLemma {
    MOCK_METHOD(lemma, derive_decision_lemma, (), (const));
};

struct MockPinResolutionLineage {
    MOCK_METHOD(void, pin, (const resolution_lineage*));
};

using test_solver_t = solver<MockSetUpSim, MockTearDownSim, MockRunSim,
    MockGetDecisionCount, MockDeriveDecisionLemma, MockPinResolutionLineage,
    MockLearnAvoidance, MockEliminationRouter>;

struct SolverTest : public ::testing::Test {
    MockSetUpSim set_up_sim;
    MockTearDownSim tear_down_sim;
    MockRunSim run_sim;
    MockGetDecisionCount get_decision_count;
    MockDeriveDecisionLemma derive_decision_lemma;
    MockPinResolutionLineage pin_resolution_lineage;
    MockLearnAvoidance learn_avoidance;
    MockEliminationRouter router;
    test_solver_t s{set_up_sim, tear_down_sim, run_sim, get_decision_count,
                 derive_decision_lemma, pin_resolution_lineage, learn_avoidance, router};
};

TEST_F(SolverTest, FirstYieldRunsSetUpThenRunBeforeTearDown) {
    testing::InSequence seq;
    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::solved));
    auto sm = s.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
}

TEST_F(SolverTest, TearDownRunsOnResumeAfterYield) {
    const lemma empty_lemma{{}};
    testing::InSequence seq;
    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(get_decision_count, count()).WillOnce(Return(0));
    EXPECT_CALL(derive_decision_lemma, derive_decision_lemma()).WillOnce(Return(empty_lemma));
    EXPECT_CALL(tear_down_sim, tear_down()).Times(1);
    EXPECT_CALL(learn_avoidance, learn(testing::_)).WillOnce(Return(std::nullopt));
    auto sm = s.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    sm.consume_yield();
    sm.resume();
}

TEST_F(SolverTest, YieldPropagatesConflictTermination) {
    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::conflicted));
    auto sm = s.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::conflicted);
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
    EXPECT_CALL(derive_decision_lemma, derive_decision_lemma())
        .WillRepeatedly(Return(empty_lemma));
    EXPECT_CALL(learn_avoidance, learn(testing::_))
        .WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(tear_down_sim, tear_down()).Times(2);

    auto sm = s.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::conflicted);
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(SolverTest, YieldPropagatesDepthExceededTermination) {
    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::depth_exceeded));
    auto sm = s.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::depth_exceeded);
}

TEST_F(SolverTest, PinsEachLemmaResolutionBeforeTearDown) {
    goal_lineage gl{nullptr, 0};
    resolution_lineage rl0{&gl, 0};
    resolution_lineage rl1{&gl, 1};
    const lemma decision_lemma{{&rl0, &rl1}};

    testing::InSequence seq;
    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(get_decision_count, count()).WillOnce(Return(0));
    EXPECT_CALL(derive_decision_lemma, derive_decision_lemma()).WillOnce(Return(decision_lemma));
    EXPECT_CALL(pin_resolution_lineage, pin(_)).Times(2);
    EXPECT_CALL(tear_down_sim, tear_down()).Times(1);
    EXPECT_CALL(learn_avoidance, learn(testing::_)).WillOnce(Return(std::nullopt));

    auto sm = s.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::conflicted);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(SolverTest, RoutesEliminationWhenLearnReturnsLineage) {
    goal_lineage gl{nullptr, 0};
    resolution_lineage elim{&gl, 1};
    const lemma empty_lemma{{}};

    EXPECT_CALL(set_up_sim, set_up()).Times(1);
    EXPECT_CALL(run_sim, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(get_decision_count, count()).WillOnce(Return(0));
    EXPECT_CALL(derive_decision_lemma, derive_decision_lemma()).WillOnce(Return(empty_lemma));
    EXPECT_CALL(tear_down_sim, tear_down()).Times(1);
    EXPECT_CALL(learn_avoidance, learn(testing::_)).WillOnce(Return(&elim));
    EXPECT_CALL(router, route(&elim)).Times(1);

    auto sm = s.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::conflicted);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

}  // namespace
