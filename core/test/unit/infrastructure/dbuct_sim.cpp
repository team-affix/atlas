// dbuct_sim: choose/at_root/terminate contracts with mocked collaborators.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstddef>
#include <vector>
#include "infrastructure/dbuct_sim.hpp"
#include "infrastructure/coroutine.hpp"
#include "value_objects/mcts_choice.hpp"

using ::testing::ByMove;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

coroutine<const resolution_lineage*, void> make_pop_stream(
    std::vector<const resolution_lineage*> elims) {
    for (const resolution_lineage* rl : elims)
        co_yield rl;
}

coroutine<const resolution_lineage*, void> empty_pop_stream() {
    co_return;
}

struct MockPushSolverFrame {
    MOCK_METHOD(void, push_solver_frame, ());
};

struct MockPopSolverFrame {
    MOCK_METHOD((coroutine<const resolution_lineage*, void>), pop_solver_frame, ());
};

struct MockGetSolverFrameDepth {
    MOCK_METHOD(size_t, depth, (), (const));
};

struct MockGetDecisionCount {
    MOCK_METHOD(size_t, count, (), (const));
};

struct MockGetPenultimateMctsFrameDepth {
    MOCK_METHOD(size_t, get_penultimate_mcts_frame_depth, (), (const));
};

struct MockGetUltimateMctsFrameDepth {
    MOCK_METHOD(size_t, get_ultimate_mcts_frame_depth, (), (const));
};

struct MockGetMctsFrameDepth {
    MOCK_METHOD(size_t, depth, (), (const));
};

struct MockBackstepMctsFrame {
    MOCK_METHOD(void, backstep, ());
};

struct MockInRollout {
    MOCK_METHOD(bool, in_rollout, (), (const));
};

struct MockChoose {
    MOCK_METHOD(mcts_choice, choose,
        (const std::vector<mcts_choice>&, const std::vector<mcts_choice>&));
};

struct MockTerminate {
    MOCK_METHOD(void, terminate, ());
};

struct MockCheckRuleChoice {
    MOCK_METHOD(bool, check_is_rule_choice, (const mcts_choice&), (const));
};

using test_dbuct_sim_t = dbuct_sim<
    mcts_choice,
    MockPushSolverFrame, MockPopSolverFrame, MockGetSolverFrameDepth,
    MockGetDecisionCount, MockGetPenultimateMctsFrameDepth, MockGetUltimateMctsFrameDepth,
    MockGetMctsFrameDepth, MockBackstepMctsFrame, MockInRollout,
    MockChoose, MockTerminate, MockCheckRuleChoice>;

struct DbuctSimTest : public ::testing::Test {
    NiceMock<MockPushSolverFrame> push_solver_frame;
    NiceMock<MockPopSolverFrame> pop_solver_frame;
    NiceMock<MockGetSolverFrameDepth> get_solver_frame_depth;
    NiceMock<MockGetDecisionCount> get_decision_count;
    NiceMock<MockGetPenultimateMctsFrameDepth> get_penultimate;
    NiceMock<MockGetUltimateMctsFrameDepth> get_ultimate;
    NiceMock<MockGetMctsFrameDepth> get_mcts_depth;
    NiceMock<MockBackstepMctsFrame> backstep;
    NiceMock<MockInRollout> in_rollout;
    NiceMock<MockChoose> choose;
    NiceMock<MockTerminate> terminate;
    NiceMock<MockCheckRuleChoice> check_rule_choice;

    test_dbuct_sim_t sim{
        push_solver_frame, pop_solver_frame, get_solver_frame_depth,
        get_decision_count, get_penultimate, get_ultimate,
        get_mcts_depth, backstep, in_rollout,
        choose, terminate, check_rule_choice};

    goal_lineage gl{nullptr, 0};
};

TEST_F(DbuctSimTest, AtRootReflectsDecisionCount) {
    EXPECT_CALL(get_decision_count, count()).WillOnce(Return(0)).WillOnce(Return(2));
    EXPECT_TRUE(sim.at_root());
    EXPECT_FALSE(sim.at_root());
}

TEST_F(DbuctSimTest, ChooseReturnsDelegatedChoice) {
    mcts_choice chosen = rule_id{7};
    std::vector<mcts_choice> choices{rule_id{7}, rule_id{8}};
    EXPECT_CALL(choose, choose(::testing::_, ::testing::_)).WillOnce(Return(chosen));
    EXPECT_CALL(get_mcts_depth, depth()).WillOnce(Return(1));
    EXPECT_CALL(get_ultimate, get_ultimate_mcts_frame_depth()).WillOnce(Return(1));

    EXPECT_EQ(sim.choose(choices), chosen);
}

TEST_F(DbuctSimTest, ChoosePushesSolverFrameForRuleChoicePastUltimate) {
    mcts_choice chosen = rule_id{3};
    std::vector<mcts_choice> choices{chosen};
    EXPECT_CALL(choose, choose(::testing::_, ::testing::_)).WillOnce(Return(chosen));
    EXPECT_CALL(get_mcts_depth, depth()).WillOnce(Return(3));
    EXPECT_CALL(get_ultimate, get_ultimate_mcts_frame_depth()).WillOnce(Return(1));
    EXPECT_CALL(check_rule_choice, check_is_rule_choice(chosen)).WillOnce(Return(true));
    EXPECT_CALL(push_solver_frame, push_solver_frame()).Times(1);

    EXPECT_EQ(sim.choose(choices), chosen);
}

TEST_F(DbuctSimTest, ChooseDoesNotPushWhenNotPastUltimate) {
    mcts_choice chosen = rule_id{3};
    std::vector<mcts_choice> choices{chosen};
    EXPECT_CALL(choose, choose(::testing::_, ::testing::_)).WillOnce(Return(chosen));
    EXPECT_CALL(get_mcts_depth, depth()).WillOnce(Return(1));
    EXPECT_CALL(get_ultimate, get_ultimate_mcts_frame_depth()).WillOnce(Return(1));
    EXPECT_CALL(push_solver_frame, push_solver_frame()).Times(0);

    EXPECT_EQ(sim.choose(choices), chosen);
}

TEST_F(DbuctSimTest, ChooseDoesNotPushForNonRuleChoice) {
    mcts_choice chosen = &gl;
    std::vector<mcts_choice> choices{chosen};
    EXPECT_CALL(choose, choose(::testing::_, ::testing::_)).WillOnce(Return(chosen));
    EXPECT_CALL(get_mcts_depth, depth()).WillOnce(Return(3));
    EXPECT_CALL(get_ultimate, get_ultimate_mcts_frame_depth()).WillOnce(Return(1));
    EXPECT_CALL(check_rule_choice, check_is_rule_choice(chosen)).WillOnce(Return(false));
    EXPECT_CALL(push_solver_frame, push_solver_frame()).Times(0);

    EXPECT_EQ(sim.choose(choices), chosen);
}

TEST_F(DbuctSimTest, TerminateEndsMctsAndReturnsSolverPopEliminations) {
    resolution_lineage rl{&gl, 0};
    EXPECT_CALL(terminate, terminate()).Times(1);
    // MCTS depth stays aligned at 1; ultimate starts deeper so one solver pop happens.
    EXPECT_CALL(get_mcts_depth, depth()).WillRepeatedly(Return(1));
    EXPECT_CALL(get_ultimate, get_ultimate_mcts_frame_depth())
        .WillOnce(Return(2))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(pop_solver_frame, pop_solver_frame())
        .WillOnce(Return(ByMove(make_pop_stream({&rl}))));
    EXPECT_CALL(backstep, backstep()).Times(0);

    EXPECT_EQ(sim.terminate(), std::vector<const resolution_lineage*>{&rl});
}

TEST_F(DbuctSimTest, TerminateWithNothingToAlignReturnsEmpty) {
    EXPECT_CALL(terminate, terminate()).Times(1);
    EXPECT_CALL(get_mcts_depth, depth()).WillRepeatedly(Return(1));
    EXPECT_CALL(get_ultimate, get_ultimate_mcts_frame_depth()).WillRepeatedly(Return(1));
    EXPECT_CALL(pop_solver_frame, pop_solver_frame()).Times(0);
    EXPECT_CALL(backstep, backstep()).Times(0);

    EXPECT_TRUE(sim.terminate().empty());
}

// Contract: each hub-pop iteration clears eliminations, so terminate() returns
// only the last pop's yields. Earlier pops may yield internally; the solver
// never sees them from this terminate() call.
//
// With mcts.depth() pinned at 1, ultimate sequences 3 → 2 → 1 force exactly two
// pops (loop: 3>1 pop, 2>1 pop, 1>1 exit), then a final ultimate read of 1.

TEST_F(DbuctSimTest, TerminateMultiPopLastPopYieldWins) {
    resolution_lineage rl_a{&gl, 0};
    resolution_lineage rl_b{&gl, 1};
    EXPECT_CALL(terminate, terminate()).Times(1);
    EXPECT_CALL(get_mcts_depth, depth()).WillRepeatedly(Return(1));
    EXPECT_CALL(get_ultimate, get_ultimate_mcts_frame_depth())
        .WillOnce(Return(3))
        .WillOnce(Return(2))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(pop_solver_frame, pop_solver_frame())
        .WillOnce(Return(ByMove(make_pop_stream({&rl_a}))))
        .WillOnce(Return(ByMove(make_pop_stream({&rl_b}))));
    EXPECT_CALL(backstep, backstep()).Times(0);

    EXPECT_EQ(sim.terminate(), std::vector<const resolution_lineage*>{&rl_b});
}

TEST_F(DbuctSimTest, TerminateMultiPopFirstYieldDiscardedWhenSecondEmpty) {
    resolution_lineage rl_a{&gl, 0};
    EXPECT_CALL(terminate, terminate()).Times(1);
    EXPECT_CALL(get_mcts_depth, depth()).WillRepeatedly(Return(1));
    EXPECT_CALL(get_ultimate, get_ultimate_mcts_frame_depth())
        .WillOnce(Return(3))
        .WillOnce(Return(2))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(pop_solver_frame, pop_solver_frame())
        .WillOnce(Return(ByMove(make_pop_stream({&rl_a}))))
        .WillOnce(Return(ByMove(empty_pop_stream())));
    EXPECT_CALL(backstep, backstep()).Times(0);

    EXPECT_TRUE(sim.terminate().empty());
}

TEST_F(DbuctSimTest, TerminateMultiPopSecondYieldReturnedWhenFirstEmpty) {
    resolution_lineage rl_b{&gl, 1};
    EXPECT_CALL(terminate, terminate()).Times(1);
    EXPECT_CALL(get_mcts_depth, depth()).WillRepeatedly(Return(1));
    EXPECT_CALL(get_ultimate, get_ultimate_mcts_frame_depth())
        .WillOnce(Return(3))
        .WillOnce(Return(2))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(pop_solver_frame, pop_solver_frame())
        .WillOnce(Return(ByMove(empty_pop_stream())))
        .WillOnce(Return(ByMove(make_pop_stream({&rl_b}))));
    EXPECT_CALL(backstep, backstep()).Times(0);

    EXPECT_EQ(sim.terminate(), std::vector<const resolution_lineage*>{&rl_b});
}

TEST_F(DbuctSimTest, TerminateMultiPopSameElimWhenBothPopYield) {
    resolution_lineage rl{&gl, 0};
    EXPECT_CALL(terminate, terminate()).Times(1);
    EXPECT_CALL(get_mcts_depth, depth()).WillRepeatedly(Return(1));
    EXPECT_CALL(get_ultimate, get_ultimate_mcts_frame_depth())
        .WillOnce(Return(3))
        .WillOnce(Return(2))
        .WillOnce(Return(1))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(pop_solver_frame, pop_solver_frame())
        .WillOnce(Return(ByMove(make_pop_stream({&rl}))))
        .WillOnce(Return(ByMove(make_pop_stream({&rl}))));
    EXPECT_CALL(backstep, backstep()).Times(0);

    EXPECT_EQ(sim.terminate(), std::vector<const resolution_lineage*>{&rl});
}

}  // namespace
