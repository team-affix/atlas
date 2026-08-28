// fgt_cdcl_elimination_generator eviction tests.
// Uses small explicit capacities so trim_to_capacity() triggers and the LRU
// fire-order semantics are directly observable.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <vector>
#include "infrastructure/fgt_cdcl_elimination_generator.hpp"
#include "infrastructure/chosen_goal_candidates.hpp"
#include "infrastructure/coroutine.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace {

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

} // namespace

struct FgtCdclEvictionTest : public ::testing::Test {
    chosen_goal_candidates chosen;
    // capacity set per test via make_cdcl(); not constructed in the fixture
    // so tests can specify their own capacity
    std::optional<fgt_cdcl_elimination_generator<chosen_goal_candidates>> cdcl;

    void make_cdcl(size_t capacity) {
        cdcl.emplace(chosen, capacity);
    }

    void end_sim() {
        cdcl->cleanup();
        chosen.clear();
    }

    goal_lineage lin_0{nullptr, 0};
    goal_lineage lin_1{nullptr, 1};
    goal_lineage lin_2{nullptr, 2};
    goal_lineage lin_3{nullptr, 3};
    goal_lineage lin_4{nullptr, 4};
    goal_lineage lin_5{nullptr, 5};
    goal_lineage lin_6{nullptr, 6};
    goal_lineage lin_7{nullptr, 7};
    goal_lineage lin_8{nullptr, 8};
    goal_lineage lin_9{nullptr, 9};

    resolution_lineage lin_0_0{&lin_0, 0};
    resolution_lineage lin_1_0{&lin_1, 1};
    resolution_lineage lin_2_0{&lin_2, 2};
    resolution_lineage lin_3_0{&lin_3, 3};
    resolution_lineage lin_4_0{&lin_4, 4};
    resolution_lineage lin_5_0{&lin_5, 5};
    resolution_lineage lin_6_0{&lin_6, 6};
    resolution_lineage lin_7_0{&lin_7, 7};
    resolution_lineage lin_8_0{&lin_8, 8};
    resolution_lineage lin_9_0{&lin_9, 9};
};

// Learning exactly capacity avoidances and cleaning up must not evict any of
// them — the store is full but not over.
TEST_F(FgtCdclEvictionTest, CleanupAtCapacityDoesNotEvict) {
    make_cdcl(2);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0}));
    end_sim();

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

// With capacity=1, learning A then B must evict A (oldest, never fired) on
// cleanup so that B survives.
TEST_F(FgtCdclEvictionTest, LearnBeyondCapacityEvictsOldestOnCleanup) {
    make_cdcl(1);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // A — oldest, never fired
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0})); // B — newest
    end_sim();

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), IsEmpty()); // A evicted
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0)); // B survives
}

// After eviction, constraining on the evicted avoidance's watched goals yields
// nothing — the watcher entry is gone.
TEST_F(FgtCdclEvictionTest, EvictedAvoidanceNoLongerConstrains) {
    make_cdcl(1);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // will be evicted
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0}));
    end_sim();

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_1_0)), IsEmpty());
}

// After eviction, the surviving avoidances still constrain correctly across
// multiple simulations.
TEST_F(FgtCdclEvictionTest, EvictionDoesNotInvalidateRemainingAvoidances) {
    make_cdcl(1);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // evicted after first cleanup
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0})); // survives

    end_sim();
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));

    end_sim();
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

// capacity=1: learn A then B; fire B during the sim (constrain B to unit);
// cleanup must evict A (never fired) and keep B (recently fired).
TEST_F(FgtCdclEvictionTest, FiredAvoidanceSurvivesOverNeverFired) {
    make_cdcl(1);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // A — never fired
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0})); // B — will fire

    // Fire B: constrain lin_2_0 causes lin_3_0 to be eliminated (B fires)
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    end_sim();

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), IsEmpty()); // A evicted
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0)); // B survives
}

// capacity=2; learn A, B, C; fire A and B every sim; across multiple sims C
// (never fired) is always the one evicted when a new one is learned.
TEST_F(FgtCdclEvictionTest, RepeatedlyFiredAvoidanceLastToBeEvicted) {
    make_cdcl(2);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // A
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0})); // B
    cdcl->learn(make_lemma({&lin_4_0, &lin_5_0})); // C — never fired, first to be evicted

    // sim 1: fire A and B
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    end_sim(); // C evicted (oldest unfired)

    // C is gone; A and B survive
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_4_0)), IsEmpty()); // C evicted
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

// Learn 5 with capacity=3; cleanup evicts the 2 oldest that never fired.
// Exactly 3 avoidances remain and constrain correctly.
TEST_F(FgtCdclEvictionTest, ExactlyCapacityAvoidancesRemainAfterCleanup) {
    make_cdcl(3);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // 1 — oldest, evicted
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0})); // 2 — evicted
    cdcl->learn(make_lemma({&lin_4_0, &lin_5_0})); // 3 — survives
    cdcl->learn(make_lemma({&lin_6_0, &lin_7_0})); // 4 — survives
    cdcl->learn(make_lemma({&lin_8_0, &lin_9_0})); // 5 — newest, survives
    end_sim();

    // 1 and 2 evicted
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), IsEmpty());
    // 3, 4, 5 survive
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_4_0)), ElementsAre(&lin_5_0));
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_6_0)), ElementsAre(&lin_7_0));
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_8_0)), ElementsAre(&lin_9_0));
}

// After eviction, surviving avoidances are fully re-registered by cleanup()
// and fire again in the next simulation.
TEST_F(FgtCdclEvictionTest, AfterEvictionCleanupRestoresSurvivors) {
    make_cdcl(1);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // evicted
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0})); // survives

    end_sim();
    // sim 2: survivor must fire again after cleanup restored its watchers
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));

    end_sim();
    // sim 3: same survivor must still fire — no double-eviction
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}
