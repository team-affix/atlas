// lp_solver differs from solver in when the root is entered and how the
// avoidance is learned: enter() and flush() run after set_up and before run,
// learn() takes no argument and returns nothing, and it must run before
// tear-down because tear-down clears the decision set learn() reads.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/lp_solver.hpp"
#include "value_objects/elimination_result.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/sim_termination.hpp"

using ::testing::ByMove;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockSetUpSim {
    MOCK_METHOD(void, set_up, (), ());
};

struct MockTearDownSim {
    MOCK_METHOD(void, tear_down, (), ());
};

struct MockRunSim {
    MOCK_METHOD(sim_termination, run, (), ());
};

struct MockGetDecisionCount {
    MOCK_METHOD(size_t, count, (), (const));
};

struct MockDeriveLemma {
    MOCK_METHOD(lemma, derive_decision_lemma, (), (const));
};

struct MockPinResolutionLineage {
    MOCK_METHOD(void, pin, (const resolution_lineage*), ());
};

struct MockLearnAvoidance {
    MOCK_METHOD(void, learn, (), ());
};

struct MockEnterDecisionFrame {
    MOCK_METHOD(void, enter, (), ());
};

struct MockFlushEliminations {
    MOCK_METHOD((coroutine<const resolution_lineage*, void>), flush, (), ());
};

struct MockEliminationRouter {
    MOCK_METHOD(elimination_result, route, (const resolution_lineage*), ());
};

coroutine<const resolution_lineage*, void> empty_flush() {
    co_return;
}

coroutine<const resolution_lineage*, void> one_flush(const resolution_lineage* rl) {
    co_yield rl;
}

} // namespace

using test_lp_solver_t = lp_solver<
    StrictMock<MockSetUpSim>, StrictMock<MockTearDownSim>, StrictMock<MockRunSim>,
    StrictMock<MockGetDecisionCount>, StrictMock<MockDeriveLemma>,
    StrictMock<MockPinResolutionLineage>, StrictMock<MockLearnAvoidance>,
    StrictMock<MockEnterDecisionFrame>, StrictMock<MockFlushEliminations>,
    StrictMock<MockEliminationRouter>>;

struct LpSolverTest : public ::testing::Test {
protected:
    StrictMock<MockSetUpSim> set_up;
    StrictMock<MockTearDownSim> tear_down;
    StrictMock<MockRunSim> run;
    StrictMock<MockGetDecisionCount> decision_count;
    StrictMock<MockDeriveLemma> derive;
    StrictMock<MockPinResolutionLineage> pin;
    StrictMock<MockLearnAvoidance> learn;
    StrictMock<MockEnterDecisionFrame> enter;
    StrictMock<MockFlushEliminations> flush;
    StrictMock<MockEliminationRouter> router;
    test_lp_solver_t solver{set_up, tear_down, run, decision_count, derive, pin,
                            learn, enter, flush, router};

    void drive() {
        auto sm = solver.solve();
        while (!sm.done())
            sm.resume();
    }

    goal_lineage g0{nullptr, 0};
    resolution_lineage g0_r0{&g0, 0};

    lemma empty_lemma{std::unordered_set<const resolution_lineage*>{}};
    lemma single_lemma{std::unordered_set<const resolution_lineage*>{&g0_r0}};
};

TEST_F(LpSolverTest, EntersAndFlushesAfterSetUpBeforeRun) {
    InSequence seq;
    EXPECT_CALL(set_up, set_up());
    EXPECT_CALL(enter, enter());
    EXPECT_CALL(flush, flush()).WillOnce(Return(ByMove(empty_flush())));
    EXPECT_CALL(run, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(decision_count, count()).WillOnce(Return(0));
    EXPECT_CALL(derive, derive_decision_lemma()).WillOnce(Return(empty_lemma));
    EXPECT_CALL(learn, learn());
    EXPECT_CALL(tear_down, tear_down());

    drive();
}

TEST_F(LpSolverTest, RootEliminationsAreRoutedBeforeRun) {
    InSequence seq;
    EXPECT_CALL(set_up, set_up());
    EXPECT_CALL(enter, enter());
    EXPECT_CALL(flush, flush()).WillOnce(Return(ByMove(one_flush(&g0_r0))));
    EXPECT_CALL(router, route(&g0_r0)).WillOnce(Return(elimination_result::added_to_backlog));
    EXPECT_CALL(run, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(decision_count, count()).WillOnce(Return(0));
    EXPECT_CALL(derive, derive_decision_lemma()).WillOnce(Return(empty_lemma));
    EXPECT_CALL(learn, learn());
    EXPECT_CALL(tear_down, tear_down());

    drive();
}

TEST_F(LpSolverTest, LearnRunsBeforeTearDown) {
    InSequence seq;
    EXPECT_CALL(set_up, set_up());
    EXPECT_CALL(enter, enter());
    EXPECT_CALL(flush, flush()).WillOnce(Return(ByMove(empty_flush())));
    EXPECT_CALL(run, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(decision_count, count()).WillOnce(Return(0));
    EXPECT_CALL(derive, derive_decision_lemma()).WillOnce(Return(empty_lemma));
    EXPECT_CALL(learn, learn());
    EXPECT_CALL(tear_down, tear_down());

    drive();
}

TEST_F(LpSolverTest, LemmaResolutionsArePinnedBeforeTearDown) {
    InSequence seq;
    EXPECT_CALL(set_up, set_up());
    EXPECT_CALL(enter, enter());
    EXPECT_CALL(flush, flush()).WillOnce(Return(ByMove(empty_flush())));
    EXPECT_CALL(run, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(decision_count, count()).WillOnce(Return(0));
    EXPECT_CALL(derive, derive_decision_lemma()).WillOnce(Return(single_lemma));
    EXPECT_CALL(pin, pin(&g0_r0));
    EXPECT_CALL(learn, learn());
    EXPECT_CALL(tear_down, tear_down());

    drive();
}

TEST_F(LpSolverTest, LoopsUntilDecisionCountIsZero) {
    InSequence seq;
    EXPECT_CALL(set_up, set_up());
    EXPECT_CALL(enter, enter());
    EXPECT_CALL(flush, flush()).WillOnce(Return(ByMove(empty_flush())));
    EXPECT_CALL(run, run()).WillOnce(Return(sim_termination::conflicted));
    EXPECT_CALL(decision_count, count()).WillOnce(Return(1));
    EXPECT_CALL(derive, derive_decision_lemma()).WillOnce(Return(empty_lemma));
    EXPECT_CALL(learn, learn());
    EXPECT_CALL(tear_down, tear_down());
    EXPECT_CALL(set_up, set_up());
    EXPECT_CALL(enter, enter());
    EXPECT_CALL(flush, flush()).WillOnce(Return(ByMove(empty_flush())));
    EXPECT_CALL(run, run()).WillOnce(Return(sim_termination::solved));
    EXPECT_CALL(decision_count, count()).WillOnce(Return(0));
    EXPECT_CALL(derive, derive_decision_lemma()).WillOnce(Return(empty_lemma));
    EXPECT_CALL(learn, learn());
    EXPECT_CALL(tear_down, tear_down());

    drive();
}
