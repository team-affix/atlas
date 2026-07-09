// dbuct_cdcl_elimination_generator is the delayed-backtracking CDCL learner. It
// derives decision lemmas, stores two-watched-literal avoidances, and re-arms /
// re-routes them as the frame frontier slides (push_frame / pop_frame) instead of
// naively rebuilding state each episode.
//
// SUT: dbuct_cdcl_elimination_generator (real). Every collaborator is a GMock
// double:
//   - ITryGetChosenGoalCandidate : try_get(goal) -> optional<rule_id>
//   - IGetUnitBoundary           : get_unit_boundary() -> size_t
//   - IDeriveDecisionLemma       : derive_decision_lemma() -> lemma
//   - IGetUltimateDecision       : get_ultimate_decision()   -> resolution_lineage*
//   - IGetPenultimateDecision    : get_penultimate_decision() -> resolution_lineage*
//
// Observability rule: the SUT exposes no getters and learn()/push_frame() are
// void, so EVERY assertion is made against the resolution_lineage* eliminations
// yielded by the only value-producing entry points -- constrain(rl) and
// pop_frame() -- plus collaborator call contracts. No internal state is inspected.

#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_cdcl_elimination_generator.hpp"
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
struct MockGetUnitBoundary {
    MOCK_METHOD(size_t, get_unit_boundary, (), (const));
};
struct MockDeriveDecisionLemma {
    MOCK_METHOD(lemma, derive_decision_lemma, ());
};
struct MockGetUltimateDecision {
    MOCK_METHOD(const resolution_lineage*, get_ultimate_decision, (), (const));
};
struct MockGetPenultimateDecision {
    MOCK_METHOD(const resolution_lineage*, get_penultimate_decision, (), (const));
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

using sut_t = dbuct_cdcl_elimination_generator<
    NiceMock<MockTryGetChosenGoalCandidate>,
    NiceMock<MockGetUnitBoundary>,
    NiceMock<MockDeriveDecisionLemma>,
    NiceMock<MockGetUltimateDecision>,
    NiceMock<MockGetPenultimateDecision>>;

struct DbuctCdclEliminationGeneratorUnitTest : public ::testing::Test {
    NiceMock<MockTryGetChosenGoalCandidate> tgcc;
    NiceMock<MockGetUnitBoundary> ub;
    NiceMock<MockDeriveDecisionLemma> dl;
    NiceMock<MockGetUltimateDecision> gud;
    NiceMock<MockGetPenultimateDecision> gpd;

    sut_t sut{tgcc, ub, dl, gud, gpd};

    // Members live on distinct top-level goals so (a) they are never ancestors of
    // one another (lemma's ancestor-removal leaves the set intact) and (b) each
    // sits on its own goal_lineage, which is what constrain/watch keys on.
    goal_lineage gU{nullptr, 0};
    resolution_lineage ult{&gU, 0};
    resolution_lineage ult_sibling{&gU, 9};  // same goal, different rule

    goal_lineage gP{nullptr, 1};
    resolution_lineage pen{&gP, 0};

    goal_lineage gX{nullptr, 2};
    resolution_lineage x{&gX, 0};

    goal_lineage gM{nullptr, 3};
    resolution_lineage m{&gM, 0};

    void SetUp() override {
        // Getter defaults: unless a test says otherwise, no goal has a committed
        // candidate (every member is "unassigned"). Call counts on these reads are
        // not part of the contract, so they are ON_CALL defaults, not EXPECT_CALLs.
        ON_CALL(tgcc, try_get(_)).WillByDefault(Return(std::optional<rule_id>{}));
        ON_CALL(ub, get_unit_boundary()).WillByDefault(Return(0u));
        ON_CALL(gud, get_ultimate_decision()).WillByDefault(Return(&ult));
        ON_CALL(gpd, get_penultimate_decision()).WillByDefault(Return(&pen));
    }

    // Drive one learn(): the lemma is fed through the derive mock (retiring so
    // sequential learns each get their own lemma), and ultimate/penultimate/boundary
    // are supplied as reads.
    void do_learn(std::initializer_list<const resolution_lineage*> members,
                  const resolution_lineage* ultimate,
                  const resolution_lineage* penultimate,
                  size_t boundary) {
        EXPECT_CALL(dl, derive_decision_lemma())
            .WillOnce(Return(make_lemma(members)))
            .RetiresOnSaturation();
        ON_CALL(ub, get_unit_boundary()).WillByDefault(Return(boundary));
        ON_CALL(gud, get_ultimate_decision()).WillByDefault(Return(ultimate));
        ON_CALL(gpd, get_penultimate_decision()).WillByDefault(Return(penultimate));
        sut.learn();
    }

    std::vector<const resolution_lineage*> constrain(const resolution_lineage* rl) {
        return collect_elims(sut.constrain(rl));
    }
    std::vector<const resolution_lineage*> pop() {
        return collect_elims(sut.pop_frame());
    }
};

// --- construction / empty ------------------------------------------------------

TEST_F(DbuctCdclEliminationGeneratorUnitTest, ConstrainOnFreshGeneratorYieldsNothing) {
    EXPECT_THAT(constrain(&ult), IsEmpty());
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, PopEmptyChildFrameYieldsNothing) {
    sut.push_frame();
    EXPECT_THAT(pop(), IsEmpty());
}

// --- learn ---------------------------------------------------------------------

TEST_F(DbuctCdclEliminationGeneratorUnitTest, LearnEmptyLemmaRecordsNothing) {
    sut.push_frame();
    EXPECT_CALL(dl, derive_decision_lemma()).WillOnce(Return(make_lemma({})));
    sut.learn();
    // No avoidance and no raised unit avoidance were recorded, so unwinding the
    // frame produces no eliminations.
    EXPECT_THAT(pop(), IsEmpty());
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, FreshlyLearnedAvoidanceIsNotWatchedYet) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/0);
    // Contract: a just-learned avoidance is raised as a unit avoidance, not armed
    // as a watched clause, so committing a member's goal before any pop yields
    // nothing.
    EXPECT_THAT(constrain(&ult), IsEmpty());
}

// --- pop_frame: raised unit avoidance ------------------------------------------

TEST_F(DbuctCdclEliminationGeneratorUnitTest, PopWhileStillUnitYieldsForcedElimination) {
    sut.push_frame();
    // boundary 0 <= depth-after-pop (1): the conflict is still unit at the parent,
    // so the forced literal (the ultimate, members[watcher_a]) is emitted.
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/0);
    EXPECT_THAT(pop(), ElementsAre(&ult));
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, StillUnitAvoidanceBubblesUpAndReemitsOnNextPop) {
    sut.push_frame();  // depth 2
    sut.push_frame();  // depth 3
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/0);
    // Still unit at depth 2: emits and bubbles the raised unit avoidance up.
    EXPECT_THAT(pop(), ElementsAre(&ult));
    // Still unit at depth 1 too: the bubbled avoidance re-emits.
    EXPECT_THAT(pop(), ElementsAre(&ult));
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, PopPastBoundaryArmsAvoidanceInsteadOfEmitting) {
    sut.push_frame();
    // boundary 5 > depth-after-pop (1): we have backtracked above the boundary, so
    // the conflict is no longer unit -- it is armed as a watched clause and nothing
    // is emitted at pop.
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());
    // Now armed: committing the ultimate forces the other watcher (penultimate).
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

// --- constrain: two-watched-literal propagation on an armed avoidance ----------

TEST_F(DbuctCdclEliminationGeneratorUnitTest, ArmedBinaryAvoidanceForcesOtherWatcher) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    ASSERT_THAT(pop(), IsEmpty());  // arm
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, FiredAvoidanceDoesNotRefireOnSameGoal) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    ASSERT_THAT(pop(), IsEmpty());
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
    // The avoidance fired and unwatched; a second commit on the same goal is a
    // no-op.
    EXPECT_THAT(constrain(&ult), IsEmpty());
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, MutuallyExclusiveSiblingSatisfiesAndUnwatches) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    ASSERT_THAT(pop(), IsEmpty());
    // A different rule on the watched goal means the avoidance can never be
    // violated on this branch: it unwatches silently.
    EXPECT_THAT(constrain(&ult_sibling), IsEmpty());
    // Having unwatched, the true ultimate no longer forces anything.
    EXPECT_THAT(constrain(&ult), IsEmpty());
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, WatcherMigratesToUnassignedMemberThenForces) {
    sut.push_frame();
    do_learn({&ult, &pen, &x}, &ult, &pen, /*boundary=*/5);
    ASSERT_THAT(pop(), IsEmpty());  // arm 3-member [ult, pen, x]

    // x's goal is unassigned, so committing the ultimate migrates the watcher onto
    // x rather than forcing -- no elimination yet.
    ON_CALL(tgcc, try_get(&gX)).WillByDefault(Return(std::optional<rule_id>{}));
    EXPECT_THAT(constrain(&ult), IsEmpty());
    // Now only x and pen are watched; committing x forces the remaining literal.
    EXPECT_THAT(constrain(&x), ElementsAre(&pen));
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, ConsistentTailMemberForcesImmediately) {
    sut.push_frame();
    do_learn({&ult, &pen, &x}, &ult, &pen, /*boundary=*/5);
    ASSERT_THAT(pop(), IsEmpty());
    // x's goal already committed to x's own rule (consistent with the conflict), so
    // scanning past it finds no new watcher: the avoidance is forced right away.
    ON_CALL(tgcc, try_get(&gX)).WillByDefault(Return(std::optional<rule_id>{x.idx}));
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, ExclusiveTailMemberSatisfiesAndUnwatches) {
    sut.push_frame();
    do_learn({&ult, &pen, &x}, &ult, &pen, /*boundary=*/5);
    ASSERT_THAT(pop(), IsEmpty());
    // x's goal committed to a DIFFERENT rule than x: the conflict is satisfied, so
    // constrain unwatches without emitting.
    ON_CALL(tgcc, try_get(&gX)).WillByDefault(Return(std::optional<rule_id>{x.idx + 1}));
    EXPECT_THAT(constrain(&ult), IsEmpty());
    EXPECT_THAT(constrain(&pen), IsEmpty());
}

// --- pop_frame: journal undo (behavioral, observed via later public calls) ------

TEST_F(DbuctCdclEliminationGeneratorUnitTest, PopUndoesUnwatchSoAvoidanceForcesAgain) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/2);
    ASSERT_THAT(pop(), IsEmpty());  // armed at the root

    sut.push_frame();
    // Firing consumes (unwatches) the avoidance within this frame.
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
    EXPECT_THAT(constrain(&ult), IsEmpty());
    // Contract: popping the frame undoes the unwatch, so the avoidance is armed
    // again and forces on the next commit.
    ASSERT_THAT(pop(), IsEmpty());
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, PopRestoresUnvisitedPartitionAfterWatcherMigration) {
    sut.push_frame();
    do_learn({&ult, &pen, &x}, &ult, &pen, /*boundary=*/2);
    ASSERT_THAT(pop(), IsEmpty());  // armed at the root

    sut.push_frame();
    EXPECT_THAT(constrain(&ult), IsEmpty());  // ult committed -> watcher migrates onto x
    ASSERT_THAT(pop(), IsEmpty());

    // Contract: pop restores the pre-frame visited/unvisited partition -- ult is OPEN
    // again. The watcher legitimately stays migrated on x (watcher identity is not
    // part of the contract; keeping it there is an intended efficiency boost), but
    // because ult is unvisited the clause is NOT unit: committing x must force
    // nothing, since scan must still see ult as an open member to fall back on.
    // (With the migration-undo bug, ult was stranded on the visited side of the
    // watchers, so committing x spuriously forced pen.)
    EXPECT_THAT(constrain(&x), IsEmpty());
}

// --- unit-lemma float ----------------------------------------------------------

TEST_F(DbuctCdclEliminationGeneratorUnitTest, SingleResolutionLemmaFloatsToPopAsElimination) {
    sut.push_frame();
    // A one-member lemma is a pure unit: the comment on learn() says it should
    // "float this elimination to the top", i.e. be emitted at pop like any other
    // still-unit conflict.
    do_learn({&m}, &m, &pen, /*boundary=*/0);
    EXPECT_THAT(pop(), ElementsAre(&m));
}

TEST_F(DbuctCdclEliminationGeneratorUnitTest, SingleResolutionLemmaStaysUnitAcrossMultiplePops) {
    sut.push_frame();  // depth 2
    sut.push_frame();  // depth 3
    // A lone unit has boundary 0, so it can never arm (an unsigned depth is never
    // < 0): it re-emits at every pop as it bubbles toward the root, and never
    // reaches the arm branch that would dereference its SIZE_MAX second watcher.
    do_learn({&m}, &m, &pen, /*boundary=*/0);
    EXPECT_THAT(pop(), ElementsAre(&m));
    EXPECT_THAT(pop(), ElementsAre(&m));
}

// --- journaling contract -------------------------------------------------------

TEST_F(DbuctCdclEliminationGeneratorUnitTest, LearnConsultsUnitBoundaryForEachStoredConflict) {
    // The boundary read is what makes a raised unit avoidance decide between
    // emit-vs-arm on pop, so learn must consult it at least once per learned
    // conflict. Exact count is an implementation detail -> AtLeast(1).
    EXPECT_CALL(ub, get_unit_boundary()).Times(AtLeast(1)).WillRepeatedly(Return(0u));
    sut.push_frame();
    EXPECT_CALL(dl, derive_decision_lemma()).WillOnce(Return(make_lemma({&ult, &pen})));
    sut.learn();
    (void)pop();
}

}  // namespace
