// fgt_bt_cdcl_elimination_generator eviction tests.
// Uses small explicit capacities so trim_to_capacity() triggers and the LRU
// fire-order semantics are directly observable.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <vector>
#include "infrastructure/fgt_bt_cdcl_elimination_generator.hpp"
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

struct FgtBtCdclEvictionTest : public ::testing::Test {
    std::optional<fgt_bt_cdcl_elimination_generator> cdcl;

    void make_cdcl(size_t capacity) {
        cdcl.emplace(capacity);
    }

    void end_sim() {
        cdcl->cleanup();
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

// Learning exactly capacity NANDs and cleaning up must not evict any of
// them — the store is full but not over.
TEST_F(FgtBtCdclEvictionTest, CleanupAtCapacityDoesNotEvict) {
    make_cdcl(2);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0}));
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0}));
    end_sim();

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), ElementsAre(&lin_1_0));
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

// With capacity=1, learning A then B must evict A (oldest, never fired) on
// cleanup so that B survives.
TEST_F(FgtBtCdclEvictionTest, LearnBeyondCapacityEvictsOldestOnCleanup) {
    make_cdcl(1);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // A — oldest, never fired
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0})); // B — newest
    end_sim();

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), IsEmpty()); // A evicted
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0)); // B survives
}

// After eviction, constraining on the evicted NAND's members yields nothing.
TEST_F(FgtBtCdclEvictionTest, EvictedNandNoLongerFires) {
    make_cdcl(1);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // will be evicted
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0}));
    end_sim();

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_1_0)), IsEmpty());
}

// After eviction, the surviving NANDs still constrain correctly across
// multiple simulations.
TEST_F(FgtBtCdclEvictionTest, EvictionDoesNotInvalidateRemainingNands) {
    make_cdcl(1);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // evicted after first cleanup
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0})); // survives

    end_sim();
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));

    end_sim();
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}

// capacity=1: learn A then B; fire B during the sim; cleanup must evict A
// (never fired) and keep B (recently fired).
TEST_F(FgtBtCdclEvictionTest, FiredNandSurvivesOverNeverFired) {
    make_cdcl(1);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // A — never fired
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0})); // B — will fire

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));
    end_sim();

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), IsEmpty()); // A evicted
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0)); // B survives
}

// capacity=2; learn A, B, C; fire A and B every sim; across multiple sims C
// (never fired) is always the one evicted when a new one is learned.
TEST_F(FgtBtCdclEvictionTest, RepeatedlyFiredNandLastToBeEvicted) {
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
// Exactly 3 NANDs remain and constrain correctly.
TEST_F(FgtBtCdclEvictionTest, ExactlyCapacityNandsRemainAfterCleanup) {
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

// After eviction, re-learning the same member-set must resurrect the NAND so
// it fires again in subsequent simulations.
TEST_F(FgtBtCdclEvictionTest, EvictedRootRelearnable) {
    make_cdcl(1);
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0})); // A — will be evicted
    cdcl->learn(make_lemma({&lin_2_0, &lin_3_0})); // B — survives
    end_sim(); // A evicted

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), IsEmpty()); // A gone

    // Re-learn A — it is a new slot in fire_order_ now
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0}));
    end_sim(); // B now evicted (oldest); A (re-learned) survives

    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), ElementsAre(&lin_1_0)); // A back
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), IsEmpty()); // B evicted
}

// A NAND root that is also a structural child of a larger NAND is not
// destroyed when the outer NAND is evicted: the inner NAND's is_avoidance
// guard in trim() stops the cascade.
TEST_F(FgtBtCdclEvictionTest, EvictingOuterNandDoesNotDestroyLiveInnerNand) {
    make_cdcl(2);
    // {A,B} is learned first; later {A,B,C,D} reuses the pair(A,B) subtree.
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0}));             // inner: {A,B}
    cdcl->learn(make_lemma({&lin_0_0, &lin_1_0, &lin_2_0, &lin_3_0})); // outer: {A,B,C,D}
    // Now learn a third NAND to push capacity to 3 (over limit of 2).
    cdcl->learn(make_lemma({&lin_4_0, &lin_5_0}));

    // capacity=2; all three were learned so fire_order_ has 3 slots
    // (pair(A,B) and outer are separate roots; pair(4,5) is a third)
    // After cleanup, the oldest — inner {A,B} — is evicted first.
    end_sim();

    // inner {A,B} evicted: constraining lin_0_0 alone no longer forces lin_1_0
    // BUT the outer {A,B,C,D} must still be alive and fire correctly.
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_0_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_1_0)), IsEmpty());
    EXPECT_THAT(collect_elims(cdcl->constrain(&lin_2_0)), ElementsAre(&lin_3_0));
}
