// dbuct_bt_cdcl_elimination_generator is delayed-backtracking CDCL over a
// hash-consed binary factor tree. Lemmas intern as unarmed NANDs; pop_frame
// either yields the stored ultimate (still unit) or arms the NAND. constrain
// increments visit counts; an armed NAND with one unvisited leaf yields it.
//
// SUT: dbuct_bt_cdcl_elimination_generator (real). Every collaborator is a GMock
// double:
//   - ITryGetChosenGoalCandidate : try_get(goal) -> optional<rule_id>
//   - IGetPenultimateMctsFrameDepth : get_penultimate_mcts_frame_depth() -> size_t
//   - IDeriveDecisionLemma       : derive_decision_lemma() -> lemma
//   - IGetUltimateDecision       : get_ultimate_decision()   -> resolution_lineage*
//   - IGetUltimateMctsFrameDepth : get_ultimate_mcts_frame_depth() -> size_t
//
// Observability rule: the SUT exposes no getters and learn()/push_frame() are
// void, so EVERY assertion is made against the resolution_lineage* eliminations
// yielded by the only value-producing entry points -- constrain(rl) and
// pop_frame() -- plus collaborator call contracts. No internal state is inspected.

#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_bt_cdcl_elimination_generator.hpp"
#include "infrastructure/coroutine.hpp"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockTryGetChosenGoalCandidate {
    MOCK_METHOD(std::optional<rule_id>, try_get, (const goal_lineage*), (const));
};
struct MockGetPenultimateMctsFrameDepth {
    MOCK_METHOD(size_t, get_penultimate_mcts_frame_depth, (), (const));
};
struct MockDeriveDecisionLemma {
    MOCK_METHOD(lemma, derive_decision_lemma, ());
};
struct MockGetUltimateDecision {
    MOCK_METHOD(const resolution_lineage*, get_ultimate_decision, (), (const));
};
struct MockGetUltimateMctsFrameDepth {
    MOCK_METHOD(size_t, get_ultimate_mcts_frame_depth, (), (const));
};

std::vector<const resolution_lineage*> collect_elims(
    coroutine<const resolution_lineage*, void> sm) {
    std::vector<const resolution_lineage*> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield()) {
            const resolution_lineage* v = sm.consume_yield();
            if (v != nullptr)
                out.push_back(v);
        }
    }
    return out;
}

lemma make_lemma(std::initializer_list<const resolution_lineage*> rs) {
    return lemma{{rs.begin(), rs.end()}};
}

using sut_t = dbuct_bt_cdcl_elimination_generator<
    NiceMock<MockTryGetChosenGoalCandidate>,
    NiceMock<MockGetPenultimateMctsFrameDepth>,
    NiceMock<MockDeriveDecisionLemma>,
    NiceMock<MockGetUltimateDecision>,
    NiceMock<MockGetUltimateMctsFrameDepth>>;

struct DbuctBtCdclEliminationGeneratorUnitTest : public ::testing::Test {
    NiceMock<MockTryGetChosenGoalCandidate> tgcc;
    NiceMock<MockGetPenultimateMctsFrameDepth> ub;
    NiceMock<MockDeriveDecisionLemma> dl;
    NiceMock<MockGetUltimateDecision> gud;
    NiceMock<MockGetUltimateMctsFrameDepth> gumfd;

    sut_t sut{tgcc, ub, dl, gud, gumfd};

    goal_lineage gU{nullptr, 0};
    resolution_lineage ult{&gU, 0};

    goal_lineage gP{nullptr, 1};
    resolution_lineage pen{&gP, 0};

    goal_lineage gX{nullptr, 2};
    resolution_lineage x{&gX, 0};

    goal_lineage gM{nullptr, 3};
    resolution_lineage m{&gM, 0};

    void SetUp() override {
        ON_CALL(tgcc, try_get(_)).WillByDefault(Return(std::optional<rule_id>{}));
        ON_CALL(ub, get_penultimate_mcts_frame_depth()).WillByDefault(Return(0u));
        ON_CALL(gud, get_ultimate_decision()).WillByDefault(Return(&ult));
        ON_CALL(gumfd, get_ultimate_mcts_frame_depth()).WillByDefault(Return(1u));
    }

    void do_learn(std::initializer_list<const resolution_lineage*> members,
                  const resolution_lineage* ultimate,
                  size_t boundary) {
        EXPECT_CALL(dl, derive_decision_lemma())
            .WillOnce(Return(make_lemma(members)))
            .RetiresOnSaturation();
        ON_CALL(ub, get_penultimate_mcts_frame_depth()).WillByDefault(Return(boundary));
        ON_CALL(gud, get_ultimate_decision()).WillByDefault(Return(ultimate));
        sut.learn();
    }

    std::vector<const resolution_lineage*> constrain(const resolution_lineage* rl) {
        return collect_elims(sut.constrain(rl));
    }
    std::vector<const resolution_lineage*> pop() {
        return collect_elims(sut.pop_frame());
    }
};

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, ConstrainOnFreshGeneratorYieldsNothing) {
    EXPECT_THAT(constrain(&ult), IsEmpty());
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, PopEmptyChildFrameYieldsNothing) {
    sut.push_frame();
    EXPECT_THAT(pop(), IsEmpty());
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, LearnEmptyLemmaRecordsNothing) {
    sut.push_frame();
    EXPECT_CALL(dl, derive_decision_lemma()).WillOnce(Return(make_lemma({})));
    sut.learn();
    EXPECT_THAT(pop(), IsEmpty());
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, FreshlyLearnedAvoidanceIsNotWatchedYet) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, /*boundary=*/0);
    EXPECT_THAT(constrain(&ult), IsEmpty());
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, PopWhileStillUnitYieldsForcedElimination) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, /*boundary=*/0);
    EXPECT_THAT(pop(), ElementsAre(&ult));
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, StillUnitAvoidanceBubblesUpAndReemitsOnNextPop) {
    sut.push_frame();
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, /*boundary=*/0);
    EXPECT_THAT(pop(), ElementsAre(&ult));
    EXPECT_THAT(pop(), ElementsAre(&ult));
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, PopPastBoundaryArmsAvoidanceInsteadOfEmitting) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, ArmedBinaryAvoidanceForcesOtherWatcher) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, /*boundary=*/5);
    ASSERT_THAT(pop(), IsEmpty());
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, FiredAvoidanceDoesNotRefireOnSameGoal) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, /*boundary=*/5);
    ASSERT_THAT(pop(), IsEmpty());
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
    EXPECT_THAT(constrain(&ult), IsEmpty());
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, ThreeMemberAvoidanceYieldsAfterTwoConstrains) {
    sut.push_frame();
    do_learn({&ult, &pen, &x}, &ult, /*boundary=*/5);
    ASSERT_THAT(pop(), IsEmpty());
    ON_CALL(tgcc, try_get(&gX)).WillByDefault(Return(std::optional<rule_id>{}));
    EXPECT_THAT(constrain(&ult), IsEmpty());
    EXPECT_THAT(constrain(&x), ElementsAre(&pen));
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, CatchUpViaTryGetAtArmTimeForcesImmediately) {
    sut.push_frame();
    do_learn({&ult, &pen, &x}, &ult, /*boundary=*/5);
    ON_CALL(tgcc, try_get(&gX)).WillByDefault(Return(std::optional<rule_id>{x.idx}));
    ASSERT_THAT(pop(), IsEmpty());
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, CatchUpViaTryGetIsUndoneOnPopSoClauseIsNotSpuriouslyUnit) {
    sut.push_frame();
    sut.push_frame();
    do_learn({&ult, &pen, &x}, &ult, /*boundary=*/5);
    ON_CALL(tgcc, try_get(&gX)).WillByDefault(Return(std::optional<rule_id>{x.idx}));
    ASSERT_THAT(pop(), IsEmpty());

    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
    ASSERT_THAT(pop(), IsEmpty());

    EXPECT_THAT(constrain(&x), IsEmpty());
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, PopUndoesFireSoAvoidanceForcesAgain) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, /*boundary=*/2);
    ASSERT_THAT(pop(), IsEmpty());

    sut.push_frame();
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
    EXPECT_THAT(constrain(&ult), IsEmpty());
    ASSERT_THAT(pop(), IsEmpty());
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, PopUndoesPartialVisitSoClauseIsNotSpuriouslyUnit) {
    sut.push_frame();
    do_learn({&ult, &pen, &x}, &ult, /*boundary=*/2);
    ASSERT_THAT(pop(), IsEmpty());

    sut.push_frame();
    EXPECT_THAT(constrain(&ult), IsEmpty());
    ASSERT_THAT(pop(), IsEmpty());

    EXPECT_THAT(constrain(&x), IsEmpty());
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, SingleResolutionLemmaFloatsToPopAsElimination) {
    sut.push_frame();
    do_learn({&m}, &m, /*boundary=*/1);
    EXPECT_THAT(pop(), ElementsAre(&m));
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, SingleResolutionLemmaStaysUnitAcrossMultiplePops) {
    sut.push_frame();
    sut.push_frame();
    do_learn({&m}, &m, /*boundary=*/1);
    EXPECT_THAT(pop(), ElementsAre(&m));
    EXPECT_THAT(pop(), ElementsAre(&m));
}

TEST_F(DbuctBtCdclEliminationGeneratorUnitTest, LearnConsultsUnitBoundaryForEachStoredConflict) {
    EXPECT_CALL(ub, get_penultimate_mcts_frame_depth()).Times(AtLeast(1)).WillRepeatedly(Return(0u));
    sut.push_frame();
    EXPECT_CALL(dl, derive_decision_lemma()).WillOnce(Return(make_lemma({&ult, &pen})));
    sut.learn();
    (void)pop();
}

}  // namespace
