// dbuct_fgt_cdcl_elimination_generator is the delayed-backtracking forgetful CDCL
// learner. It extends the DBUCT CDCL with an LRU eviction pool: avoidances only
// enter the pool when they de-camp (pop_frame re-arm branch). Camped-under
// avoidances are excluded from the pool. Avoidances that go unit during constrain
// are temporarily removed from the pool (made unevictable) and reinstated on
// pop_frame via undo.
//
// SUT: dbuct_fgt_cdcl_elimination_generator (real). Every collaborator is a GMock
// double (same mocks as the base dbuct_cdcl tests).
//
// Observability: we assert exclusively against yielded resolution_lineage* values
// from constrain() and pop_frame(). No internal state is inspected.

#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/dbuct_fgt_cdcl_elimination_generator.hpp"
#include "infrastructure/coroutine.hpp"

using ::testing::_;
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
struct MockGetPenultimateDecision {
    MOCK_METHOD(const resolution_lineage*, get_penultimate_decision, (), (const));
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

using sut_t = dbuct_fgt_cdcl_elimination_generator<
    NiceMock<MockTryGetChosenGoalCandidate>,
    NiceMock<MockGetPenultimateMctsFrameDepth>,
    NiceMock<MockDeriveDecisionLemma>,
    NiceMock<MockGetUltimateDecision>,
    NiceMock<MockGetPenultimateDecision>,
    NiceMock<MockGetUltimateMctsFrameDepth>>;

struct DbuctFgtCdclEliminationGeneratorUnitTest : public ::testing::Test {
    NiceMock<MockTryGetChosenGoalCandidate> tgcc;
    NiceMock<MockGetPenultimateMctsFrameDepth> ub;
    NiceMock<MockDeriveDecisionLemma> dl;
    NiceMock<MockGetUltimateDecision> gud;
    NiceMock<MockGetPenultimateDecision> gpd;
    NiceMock<MockGetUltimateMctsFrameDepth> gumfd;

    static constexpr size_t k_capacity = 2;
    sut_t sut{tgcc, ub, dl, gud, gpd, gumfd, k_capacity};

    goal_lineage gU{nullptr, 0};
    resolution_lineage ult{&gU, 0};
    resolution_lineage ult_sibling{&gU, 9};

    goal_lineage gP{nullptr, 1};
    resolution_lineage pen{&gP, 0};

    goal_lineage gX{nullptr, 2};
    resolution_lineage x{&gX, 0};

    goal_lineage gM{nullptr, 3};
    resolution_lineage m{&gM, 0};

    goal_lineage gN{nullptr, 4};
    resolution_lineage n{&gN, 0};

    void SetUp() override {
        ON_CALL(tgcc, try_get(_)).WillByDefault(Return(std::optional<rule_id>{}));
        ON_CALL(ub, get_penultimate_mcts_frame_depth()).WillByDefault(Return(0u));
        ON_CALL(gud, get_ultimate_decision()).WillByDefault(Return(&ult));
        ON_CALL(gpd, get_penultimate_decision()).WillByDefault(Return(&pen));
        ON_CALL(gumfd, get_ultimate_mcts_frame_depth()).WillByDefault(Return(1u));
    }

    void do_learn(std::initializer_list<const resolution_lineage*> members,
                  const resolution_lineage* ultimate,
                  const resolution_lineage* penultimate,
                  size_t boundary) {
        EXPECT_CALL(dl, derive_decision_lemma())
            .WillOnce(Return(make_lemma(members)))
            .RetiresOnSaturation();
        ON_CALL(ub, get_penultimate_mcts_frame_depth()).WillByDefault(Return(boundary));
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

// --- basic dbuct behaviour still works -----------------------------------------

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, ConstrainOnFreshGeneratorYieldsNothing) {
    EXPECT_THAT(constrain(&ult), IsEmpty());
}

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, PopEmptyChildFrameYieldsNothing) {
    sut.push_frame();
    EXPECT_THAT(pop(), IsEmpty());
}

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, FreshlyLearnedAvoidanceIsNotWatchedYet) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/0);
    EXPECT_THAT(constrain(&ult), IsEmpty());
}

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, PopWhileStillUnitYieldsForcedElimination) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/0);
    EXPECT_THAT(pop(), ElementsAre(&ult));
}

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, PopPastBoundaryArmsAvoidanceAndAddsToPool) {
    sut.push_frame();
    // ultimate_mcts (1) < boundary (5): de-camps and is added to the evictable pool.
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());
    // Now armed: committing the ultimate forces the other watcher.
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

// --- fire_order_: pool not touched until de-camp --------------------------------

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, NewlyLearnedAvoidanceDoesNotEnterPoolWhileUnitBubbling) {
    sut.push_frame();  // depth 2
    sut.push_frame();  // depth 3
    // boundary=0: the avoidance is camping at every depth (ultimate_mcts >= 0 is always true)
    // so it always stays in the lump and never de-camps.
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/0);
    EXPECT_THAT(pop(), ElementsAre(&ult));
    // Still camping: bubbled up. constrain cannot see it.
    EXPECT_THAT(constrain(&ult), IsEmpty());
    EXPECT_THAT(pop(), ElementsAre(&ult));
}

// --- capacity enforcement ------------------------------------------------------

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, AtCapacityDeCampNoEviction) {
    // capacity=2; de-camp two avoidances. No eviction should occur since pool
    // reaches but does not exceed capacity.
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    sut.push_frame();
    do_learn({&x, &m}, &x, &m, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // de-camps second
    EXPECT_THAT(pop(), IsEmpty());  // de-camps first
    // Both are armed; both can force.
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
    EXPECT_THAT(constrain(&x), ElementsAre(&m));
}

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, BeyondCapacityDeCampEvictsLruAvoidance) {
    // capacity=2. De-camp sequentially so each avoidance enters the pool as it
    // leaves unit territory. The first to de-camp is LRU and is evicted when the
    // third de-camps.
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // pool: [ult+pen]

    sut.push_frame();
    do_learn({&x, &m}, &x, &m, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // pool: [ult+pen, x+m]

    sut.push_frame();
    do_learn({&n, &m}, &n, &m, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // evicts ult+pen; pool: [x+m, n+m]

    EXPECT_THAT(constrain(&ult), IsEmpty());
    EXPECT_THAT(constrain(&x), ElementsAre(&m));
    EXPECT_THAT(constrain(&n), ElementsAre(&m));
}

// --- make_unevictable: avoidance that goes unit is removed from pool -----------

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, UnitAvoidanceRemovedFromPoolOnConstrain) {
    // De-camp one avoidance (adds to pool). Then constrain to make it unit.
    // On pop, the undo of make_unevictable should reinstate it to the pool.
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // armed, in pool

    sut.push_frame();
    // Commit ultimate: avoidance goes unit, removed from pool.
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));

    // Pop: undo of make_unevictable reinstates it to pool. Undo of unwatch re-arms.
    EXPECT_THAT(pop(), IsEmpty());

    // Re-armed and back in pool: fires again.
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

// --- camped-under avoidance: never in pool -------------------------------------

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, CampedUnderAvoidanceNeverEntersPool) {
    // boundary=0: avoidance always camps. Even across many frames it never de-camps
    // and is never added to the evictable pool.
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/0);

    // Pop while still camping: emits but does not add to pool.
    EXPECT_THAT(pop(), ElementsAre(&ult));

    // After bubbling up to root frame, constrain should still see nothing (not armed).
    EXPECT_THAT(constrain(&ult), IsEmpty());
}

// --- evicted avoidance no longer fires -----------------------------------------

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, EvictedAvoidanceNoLongerFires) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());

    sut.push_frame();
    do_learn({&x, &m}, &x, &m, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());

    sut.push_frame();
    do_learn({&n, &m}, &n, &m, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());

    EXPECT_THAT(constrain(&ult), IsEmpty());
    EXPECT_THAT(constrain(&pen), IsEmpty());
}

// --- undo restores pool after unwatch in child frame ---------------------------

TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, PopUndoesUnwatchSoAvoidanceForcesAgain) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/2);
    ASSERT_THAT(pop(), IsEmpty());  // armed at root, in pool

    sut.push_frame();
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
    EXPECT_THAT(constrain(&ult), IsEmpty());
    ASSERT_THAT(pop(), IsEmpty());  // undo: re-armed, back in pool
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
}

// An avoidance that went unit is made unevictable, so a later de-camp that
// overflows the pool evicts a never-unit neighbour instead. On pop, the unit
// avoidance is made evictable at the back (freshest) and the never-unit one
// is gone.
TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, RecentlyUnitAvoidanceSurvivesOverNeverUnit) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // pool: [ult+pen]

    sut.push_frame();
    do_learn({&x, &m}, &x, &m, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // pool: [ult+pen, x+m]

    sut.push_frame();
    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));  // ult+pen unevictable; pool: [x+m]

    sut.push_frame();
    do_learn({&n, &m}, &n, &m, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // pool: [x+m, n+m]

    EXPECT_THAT(pop(), IsEmpty());  // make_evictable(ult+pen) evicts x+m; pool: [n+m, ult+pen]

    EXPECT_THAT(constrain(&ult), ElementsAre(&pen));
    EXPECT_THAT(constrain(&n), ElementsAre(&m));
    EXPECT_THAT(constrain(&x), IsEmpty());
}

// A violated (dead) avoidance is unwatched but stays in the LRU list. It can
// therefore be evicted by a later de-camp, and the unwatch undo must no-op.
TEST_F(DbuctFgtCdclEliminationGeneratorUnitTest, ViolatedAvoidanceStaysEvictableAndCanBeLru) {
    sut.push_frame();
    do_learn({&ult, &pen}, &ult, &pen, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // pool: [ult+pen]

    sut.push_frame();
    EXPECT_THAT(constrain(&ult_sibling), IsEmpty());  // unwatch, still in pool

    sut.push_frame();
    do_learn({&x, &m}, &x, &m, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // pool: [ult+pen, x+m]

    sut.push_frame();
    do_learn({&n, &m}, &n, &m, /*boundary=*/5);
    EXPECT_THAT(pop(), IsEmpty());  // evicts ult+pen; pool: [x+m, n+m]

    EXPECT_THAT(pop(), IsEmpty());  // unwatch undo of evicted ult+pen is a no-op

    EXPECT_THAT(constrain(&ult), IsEmpty());
    EXPECT_THAT(constrain(&x), ElementsAre(&m));
    EXPECT_THAT(constrain(&n), ElementsAre(&m));
}

}  // namespace
