#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <set>
#include <vector>
#include "infrastructure/incremental_set.hpp"

// Instrumented key: counts operator== and operator< calls so tests can
// verify that contains() traverses O(log n) nodes rather than O(n).
static int g_cmp_count = 0;

struct cmp_key {
    uint32_t value;
    bool operator==(const cmp_key& o) const { ++g_cmp_count; return value == o.value; }
    bool operator<(const cmp_key& o)  const { ++g_cmp_count; return value < o.value;  }
};

namespace std {
template<> struct hash<cmp_key> {
    size_t operator()(const cmp_key& k) const { return std::hash<uint32_t>{}(k.value); }
};
} // namespace std

namespace {

using set_u32 = incremental_set<uint32_t>;

void expect_contains_exactly(const set_u32& s, const std::vector<uint32_t>& keys) {
    for (uint32_t key : keys)
        EXPECT_TRUE(s.contains(key)) << "expected to contain " << key;
}

void expect_absent(const set_u32& s, const std::vector<uint32_t>& keys) {
    for (uint32_t key : keys)
        EXPECT_FALSE(s.contains(key)) << "expected NOT to contain " << key;
}

std::vector<uint32_t> collect(const set_u32& s) {
    std::vector<uint32_t> result;
    for (uint32_t key : s)
        result.push_back(key);
    return result;
}

} // namespace

struct IncrementalSetTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// Default-constructed set
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, DefaultSetContainsFalseForAnyKey) {
    set_u32 s;
    EXPECT_FALSE(s.contains(0));
    EXPECT_FALSE(s.contains(1));
    EXPECT_FALSE(s.contains(0xffffffffu));
}

TEST_F(IncrementalSetTest, EraseFromDefaultSetDoesNotContainKey) {
    set_u32 s;
    const set_u32 after = s.erase(42u);
    EXPECT_FALSE(after.contains(42u));
}

// ---------------------------------------------------------------------------
// Insert first key
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, InsertFirstKeyContainsIt) {
    set_u32 s;
    const set_u32 a = s.insert(7u);
    EXPECT_TRUE(a.contains(7u));
    EXPECT_FALSE(a.contains(0u));
    EXPECT_FALSE(a.contains(8u));
}

TEST_F(IncrementalSetTest, InsertFirstKeyReceiverStillEmpty) {
    set_u32 s;
    const set_u32 a = s.insert(7u);
    EXPECT_FALSE(s.contains(7u));
    (void)a;
}

// ---------------------------------------------------------------------------
// Duplicate insert
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, DuplicateInsertReceiverUnchanged) {
    set_u32 s = set_u32{}.insert(5u);
    const set_u32 a = s.insert(5u);
    EXPECT_TRUE(a.contains(5u));
    EXPECT_TRUE(s.contains(5u));
}

TEST_F(IncrementalSetTest, DuplicateInsertDoesNotAddExtraElement) {
    set_u32 s = set_u32{}.insert(5u).insert(5u);
    EXPECT_TRUE(s.contains(5u));
    EXPECT_FALSE(s.contains(4u));
    EXPECT_FALSE(s.contains(6u));
}

// ---------------------------------------------------------------------------
// Boundary keys
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, KeyZeroAlone) {
    const set_u32 s = set_u32{}.insert(0u);
    EXPECT_TRUE(s.contains(0u));
    EXPECT_FALSE(s.contains(1u));
}

TEST_F(IncrementalSetTest, KeyMaxUint32Alone) {
    const set_u32 s = set_u32{}.insert(0xffffffffu);
    EXPECT_TRUE(s.contains(0xffffffffu));
    EXPECT_FALSE(s.contains(0u));
}

TEST_F(IncrementalSetTest, KeyZeroAndKeyMaxTogether) {
    const set_u32 s = set_u32{}.insert(0u).insert(0xffffffffu);
    EXPECT_TRUE(s.contains(0u));
    EXPECT_TRUE(s.contains(0xffffffffu));
    EXPECT_FALSE(s.contains(1u));
}

// ---------------------------------------------------------------------------
// Two keys in both insertion orders
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, TwoKeysInsertLowThenHigh) {
    const set_u32 s = set_u32{}.insert(10u).insert(20u);
    EXPECT_TRUE(s.contains(10u));
    EXPECT_TRUE(s.contains(20u));
    EXPECT_FALSE(s.contains(15u));
}

TEST_F(IncrementalSetTest, TwoKeysInsertHighThenLow) {
    const set_u32 s = set_u32{}.insert(20u).insert(10u);
    EXPECT_TRUE(s.contains(10u));
    EXPECT_TRUE(s.contains(20u));
    EXPECT_FALSE(s.contains(15u));
}

// ---------------------------------------------------------------------------
// Erase operations
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, EraseOnlyElementYieldsEmpty) {
    const set_u32 with_one = set_u32{}.insert(42u);
    const set_u32 empty    = with_one.erase(42u);
    EXPECT_FALSE(empty.contains(42u));
    EXPECT_TRUE(with_one.contains(42u));
}

TEST_F(IncrementalSetTest, EraseAbsentKeyLeavesContentsUnchanged) {
    const set_u32 s = set_u32{}.insert(1u).insert(2u);
    const set_u32 r = s.erase(99u);
    EXPECT_TRUE(r.contains(1u));
    EXPECT_TRUE(r.contains(2u));
    EXPECT_FALSE(r.contains(99u));
}

TEST_F(IncrementalSetTest, EraseMinimumKey) {
    const set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u);
    const set_u32 r = s.erase(1u);
    EXPECT_FALSE(r.contains(1u));
    EXPECT_TRUE(r.contains(2u));
    EXPECT_TRUE(r.contains(3u));
    expect_contains_exactly(s, {1u, 2u, 3u});
}

TEST_F(IncrementalSetTest, EraseMaximumKey) {
    const set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u);
    const set_u32 r = s.erase(3u);
    EXPECT_TRUE(r.contains(1u));
    EXPECT_TRUE(r.contains(2u));
    EXPECT_FALSE(r.contains(3u));
    expect_contains_exactly(s, {1u, 2u, 3u});
}

TEST_F(IncrementalSetTest, EraseMiddleKey) {
    const set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u);
    const set_u32 r = s.erase(2u);
    EXPECT_TRUE(r.contains(1u));
    EXPECT_FALSE(r.contains(2u));
    EXPECT_TRUE(r.contains(3u));
    expect_contains_exactly(s, {1u, 2u, 3u});
}

TEST_F(IncrementalSetTest, EraseAllElementsInSortedOrder) {
    set_u32 s = set_u32{}.insert(10u).insert(20u).insert(30u).insert(40u);
    const set_u32 original = s;

    s = s.erase(10u);
    EXPECT_FALSE(s.contains(10u)); EXPECT_TRUE(s.contains(20u));
    s = s.erase(20u);
    EXPECT_FALSE(s.contains(20u)); EXPECT_TRUE(s.contains(30u));
    s = s.erase(30u);
    EXPECT_FALSE(s.contains(30u)); EXPECT_TRUE(s.contains(40u));
    s = s.erase(40u);
    EXPECT_FALSE(s.contains(40u));

    expect_contains_exactly(original, {10u, 20u, 30u, 40u});
}

TEST_F(IncrementalSetTest, EraseAllElementsInReverseOrder) {
    set_u32 s = set_u32{}.insert(10u).insert(20u).insert(30u).insert(40u);
    const set_u32 original = s;

    s = s.erase(40u);
    EXPECT_FALSE(s.contains(40u)); EXPECT_TRUE(s.contains(10u));
    s = s.erase(30u);
    EXPECT_FALSE(s.contains(30u));
    s = s.erase(20u);
    EXPECT_FALSE(s.contains(20u));
    s = s.erase(10u);
    EXPECT_FALSE(s.contains(10u));

    expect_contains_exactly(original, {10u, 20u, 30u, 40u});
}

TEST_F(IncrementalSetTest, ReinsertAfterErase) {
    const set_u32 with_key  = set_u32{}.insert(7u);
    const set_u32 without   = with_key.erase(7u);
    const set_u32 reinserted = without.insert(7u);

    EXPECT_TRUE(with_key.contains(7u));
    EXPECT_FALSE(without.contains(7u));
    EXPECT_TRUE(reinserted.contains(7u));
}

// ---------------------------------------------------------------------------
// Sorted, reverse, and shuffled inserts
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, SortedInsertContainsAll) {
    constexpr uint32_t k_n = 50u;
    set_u32 s;
    for (uint32_t i = 0; i < k_n; ++i)
        s = s.insert(i);
    for (uint32_t i = 0; i < k_n; ++i)
        EXPECT_TRUE(s.contains(i)) << "missing key " << i;
    EXPECT_FALSE(s.contains(k_n));
}

TEST_F(IncrementalSetTest, ReverseInsertContainsAll) {
    constexpr uint32_t k_n = 50u;
    set_u32 s;
    for (uint32_t i = k_n; i-- > 0;)
        s = s.insert(i);
    for (uint32_t i = 0; i < k_n; ++i)
        EXPECT_TRUE(s.contains(i)) << "missing key " << i;
}

TEST_F(IncrementalSetTest, ShuffledInsertContainsAll) {
    constexpr uint32_t k_n = 50u;
    std::vector<uint32_t> keys(k_n);
    for (uint32_t i = 0; i < k_n; ++i) keys[i] = i;
    std::mt19937 rng(42);
    std::shuffle(keys.begin(), keys.end(), rng);

    set_u32 s;
    for (uint32_t k : keys) s = s.insert(k);
    for (uint32_t i = 0; i < k_n; ++i)
        EXPECT_TRUE(s.contains(i)) << "missing key " << i;
}

// ---------------------------------------------------------------------------
// Branching (structural sharing)
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, TwoSetsBranchedFromParentEachHasOnlyOwnKey) {
    const set_u32 parent;
    const set_u32 left  = parent.insert(1u);
    const set_u32 right = parent.insert(2u);

    EXPECT_TRUE(left.contains(1u));
    EXPECT_FALSE(left.contains(2u));
    EXPECT_FALSE(right.contains(1u));
    EXPECT_TRUE(right.contains(2u));
    EXPECT_FALSE(parent.contains(1u));
    EXPECT_FALSE(parent.contains(2u));
}

TEST_F(IncrementalSetTest, FurtherInsertOnOneBranchInvisibleToOther) {
    const set_u32 parent = set_u32{}.insert(10u);
    const set_u32 b1 = parent.insert(20u);
    const set_u32 b2 = parent.insert(30u);
    const set_u32 b1_extended = b1.insert(40u);

    EXPECT_TRUE(b1.contains(10u));  EXPECT_TRUE(b1.contains(20u));
    EXPECT_FALSE(b1.contains(30u)); EXPECT_FALSE(b1.contains(40u));
    EXPECT_TRUE(b2.contains(10u));  EXPECT_TRUE(b2.contains(30u));
    EXPECT_FALSE(b2.contains(20u)); EXPECT_FALSE(b2.contains(40u));
    EXPECT_TRUE(b1_extended.contains(40u));
    EXPECT_FALSE(b2.contains(40u));
}

TEST_F(IncrementalSetTest, ThreeSetsFromOneParentIndependent) {
    const set_u32 parent = set_u32{}.insert(5u);
    const set_u32 a = parent.insert(1u);
    const set_u32 b = parent.erase(5u);
    const set_u32 c = parent;

    EXPECT_TRUE(a.contains(5u));  EXPECT_TRUE(a.contains(1u));
    EXPECT_FALSE(b.contains(5u));
    EXPECT_TRUE(c.contains(5u));  EXPECT_FALSE(c.contains(1u));
}

TEST_F(IncrementalSetTest, ChainRetainedEarlySetUnchangedAfterManyOps) {
    set_u32 s;
    for (uint32_t i = 0; i < 20u; ++i) s = s.insert(i);
    const set_u32 early = s;

    for (uint32_t i = 20u; i < 100u; ++i) s = s.insert(i);
    for (uint32_t i = 0u; i < 50u; i += 2) s = s.erase(i);

    for (uint32_t i = 0; i < 20u; ++i)
        EXPECT_TRUE(early.contains(i)) << "early set lost key " << i;
    EXPECT_FALSE(early.contains(20u));
}

TEST_F(IncrementalSetTest, CopyThenInsertOriginalUnchanged) {
    const set_u32 original = set_u32{}.insert(3u).insert(7u);
    const set_u32 copy = original;
    const set_u32 extended = copy.insert(11u);

    EXPECT_FALSE(original.contains(11u));
    EXPECT_TRUE(extended.contains(11u));
    EXPECT_TRUE(extended.contains(3u));
}

// ---------------------------------------------------------------------------
// Receiver unchanged after erase
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, ReceiverUnchangedAfterErase) {
    const set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u);
    const set_u32 r = s.erase(2u);
    expect_contains_exactly(s, {1u, 2u, 3u});
    EXPECT_FALSE(r.contains(2u));
}

// ---------------------------------------------------------------------------
// Stress test 1: sorted insert then erase, original retained
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, StressSortedInsertEraseEveryOtherThenRest) {
    constexpr uint32_t k_n = 2000u;

    set_u32 s;
    for (uint32_t i = 0; i < k_n; ++i) s = s.insert(i);

    const set_u32 full = s;

    // erase every other key
    std::set<uint32_t> oracle;
    for (uint32_t i = 0; i < k_n; ++i) oracle.insert(i);

    for (uint32_t i = 0; i < k_n; i += 2) {
        s = s.erase(i);
        oracle.erase(i);
        for (uint32_t k = 0; k < k_n; ++k) {
            const bool expected = oracle.count(k) > 0;
            ASSERT_EQ(s.contains(k), expected)
                << "after erasing even keys up to " << i << ", key " << k;
        }
    }

    // erase remaining odd keys
    for (uint32_t i = 1; i < k_n; i += 2) {
        s = s.erase(i);
        oracle.erase(i);
    }
    for (uint32_t k = 0; k < k_n; ++k)
        ASSERT_FALSE(s.contains(k)) << "key " << k << " should be gone";

    // full set still intact
    for (uint32_t i = 0; i < k_n; ++i)
        ASSERT_TRUE(full.contains(i)) << "full set lost key " << i;
}

// ---------------------------------------------------------------------------
// Stress test 2: 32 live sets, random insert/erase/branch
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, StressRandomOpsMultipleLiveSets) {
    constexpr int k_live_count = 32;
    constexpr int k_ops        = 4000;
    constexpr uint32_t k_key_range = 400u;
    constexpr uint32_t k_seed = 12345u;

    std::mt19937 rng(k_seed);

    std::vector<set_u32>          live(k_live_count);
    std::vector<std::set<uint32_t>> oracles(k_live_count);

    std::uniform_int_distribution<int>      pick_set(0, k_live_count - 1);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_key_range - 1u);
    std::uniform_int_distribution<int>      pick_op(0, 2); // 0=insert,1=erase,2=branch

    for (int step = 0; step < k_ops; ++step) {
        const int src_idx = pick_set(rng);
        const int op      = pick_op(rng);
        const uint32_t key = pick_key(rng);

        if (op == 2) {
            // branch: copy src into a random destination
            const int dst_idx = pick_set(rng);
            live[dst_idx]    = live[src_idx];
            oracles[dst_idx] = oracles[src_idx];
        } else {
            if (op == 0) {
                live[src_idx]    = live[src_idx].insert(key);
                oracles[src_idx].insert(key);
            } else {
                live[src_idx]    = live[src_idx].erase(key);
                oracles[src_idx].erase(key);
            }
        }

        // verify ALL live sets on every step
        for (int idx = 0; idx < k_live_count; ++idx) {
            for (uint32_t k = 0; k < k_key_range; ++k) {
                const bool expected = oracles[idx].count(k) > 0;
                ASSERT_EQ(live[idx].contains(k), expected)
                    << "step " << step << " set " << idx << " key " << k;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Stress test 3: retained every-10th set re-checked at end
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, StressRetainedSetsCorrectAfterLongLineage) {
    constexpr int k_ops       = 4000;
    constexpr uint32_t k_key_range = 400u;
    constexpr uint32_t k_seed = 99999u;

    std::mt19937 rng(k_seed);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_key_range - 1u);
    std::uniform_int_distribution<int>      pick_op(0, 1);

    std::vector<set_u32>          snapshots;
    std::vector<std::set<uint32_t>> snapshot_oracles;

    set_u32 s;
    std::set<uint32_t> oracle;

    for (int step = 0; step < k_ops; ++step) {
        const uint32_t key = pick_key(rng);
        if (pick_op(rng) == 0) {
            s = s.insert(key);
            oracle.insert(key);
        } else {
            s = s.erase(key);
            oracle.erase(key);
        }
        if (step % 10 == 9) {
            snapshots.push_back(s);
            snapshot_oracles.push_back(oracle);
        }
    }

    for (size_t i = 0; i < snapshots.size(); ++i) {
        for (uint32_t k = 0; k < k_key_range; ++k) {
            const bool expected = snapshot_oracles[i].count(k) > 0;
            ASSERT_EQ(snapshots[i].contains(k), expected)
                << "snapshot " << i << " key " << k;
        }
    }
}

// ---------------------------------------------------------------------------
// Iterator: empty set
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, IteratorEmptySetBeginEqualsEnd) {
    const set_u32 s;
    EXPECT_EQ(s.begin(), s.end());
}

TEST_F(IncrementalSetTest, IteratorEmptySetCollectIsEmpty) {
    const set_u32 s;
    EXPECT_TRUE(collect(s).empty());
}

// ---------------------------------------------------------------------------
// Iterator: single element
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, IteratorSingleElementDeref) {
    const set_u32 s = set_u32{}.insert(42u);
    EXPECT_EQ(*s.begin(), 42u);
}

TEST_F(IncrementalSetTest, IteratorSingleElementBeginNotEqualEnd) {
    const set_u32 s = set_u32{}.insert(42u);
    EXPECT_NE(s.begin(), s.end());
}

TEST_F(IncrementalSetTest, IteratorSingleElementIncrementReachesEnd) {
    const set_u32 s = set_u32{}.insert(42u);
    auto it = s.begin();
    ++it;
    EXPECT_EQ(it, s.end());
}

TEST_F(IncrementalSetTest, IteratorSingleElementCollect) {
    const set_u32 s = set_u32{}.insert(99u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{99u}));
}

// ---------------------------------------------------------------------------
// Iterator: sorted order
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, IteratorTwoKeysInsertedLowHighSortedOrder) {
    const set_u32 s = set_u32{}.insert(10u).insert(20u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{10u, 20u}));
}

TEST_F(IncrementalSetTest, IteratorTwoKeysInsertedHighLowSortedOrder) {
    const set_u32 s = set_u32{}.insert(20u).insert(10u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{10u, 20u}));
}

TEST_F(IncrementalSetTest, IteratorThreeKeysSortedOrder) {
    const set_u32 s = set_u32{}.insert(30u).insert(10u).insert(20u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{10u, 20u, 30u}));
}

TEST_F(IncrementalSetTest, IteratorBoundaryKeysSortedOrder) {
    const set_u32 s = set_u32{}.insert(0xffffffffu).insert(0u).insert(1u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{0u, 1u, 0xffffffffu}));
}

TEST_F(IncrementalSetTest, IteratorSortedInsertProducesSortedOutput) {
    constexpr uint32_t k_n = 20u;
    set_u32 s;
    for (uint32_t i = 0; i < k_n; ++i)
        s = s.insert(i);
    const std::vector<uint32_t> result = collect(s);
    ASSERT_EQ(result.size(), k_n);
    for (uint32_t i = 0; i < k_n; ++i)
        EXPECT_EQ(result[i], i) << "position " << i;
}

TEST_F(IncrementalSetTest, IteratorReverseInsertProducesSortedOutput) {
    constexpr uint32_t k_n = 20u;
    set_u32 s;
    for (uint32_t i = k_n; i-- > 0;)
        s = s.insert(i);
    const std::vector<uint32_t> result = collect(s);
    ASSERT_EQ(result.size(), k_n);
    for (uint32_t i = 0; i < k_n; ++i)
        EXPECT_EQ(result[i], i) << "position " << i;
}

// ---------------------------------------------------------------------------
// Iterator: after erase
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, IteratorAfterEraseOnlyElementIsEmpty) {
    const set_u32 s = set_u32{}.insert(7u).erase(7u);
    EXPECT_EQ(s.begin(), s.end());
    EXPECT_TRUE(collect(s).empty());
}

TEST_F(IncrementalSetTest, IteratorAfterEraseMinKeyProducesSortedRemainder) {
    const set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u).erase(1u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{2u, 3u}));
}

TEST_F(IncrementalSetTest, IteratorAfterEraseMaxKeyProducesSortedRemainder) {
    const set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u).erase(3u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{1u, 2u}));
}

TEST_F(IncrementalSetTest, IteratorAfterEraseMiddleKeyProducesSortedRemainder) {
    const set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u).erase(2u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{1u, 3u}));
}

// ---------------------------------------------------------------------------
// Iterator: branched sets iterate independently
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, IteratorBranchedSetsIterateIndependently) {
    const set_u32 parent = set_u32{}.insert(5u).insert(10u);
    const set_u32 left   = parent.insert(1u);
    const set_u32 right  = parent.insert(20u);

    EXPECT_EQ(collect(parent), (std::vector<uint32_t>{5u, 10u}));
    EXPECT_EQ(collect(left),   (std::vector<uint32_t>{1u, 5u, 10u}));
    EXPECT_EQ(collect(right),  (std::vector<uint32_t>{5u, 10u, 20u}));
}

TEST_F(IncrementalSetTest, IteratorIteratingOneBranchDoesNotAffectOther) {
    const set_u32 parent = set_u32{}.insert(3u).insert(7u);
    const set_u32 branch = parent.insert(1u);

    std::vector<uint32_t> parent_keys = collect(parent);
    std::vector<uint32_t> branch_keys = collect(branch);

    EXPECT_EQ(parent_keys, (std::vector<uint32_t>{3u, 7u}));
    EXPECT_EQ(branch_keys, (std::vector<uint32_t>{1u, 3u, 7u}));
}

// ---------------------------------------------------------------------------
// Iterator stress: oracle comparison against std::set
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Iterator: std::distance matches element count
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, IteratorDistanceEmptySetIsZero) {
    const set_u32 s;
    EXPECT_EQ(std::distance(s.begin(), s.end()), 0);
}

TEST_F(IncrementalSetTest, IteratorDistanceSingleElementIsOne) {
    const set_u32 s = set_u32{}.insert(7u);
    EXPECT_EQ(std::distance(s.begin(), s.end()), 1);
}

TEST_F(IncrementalSetTest, IteratorDistanceMatchesInsertCount) {
    constexpr uint32_t k_n = 15u;
    set_u32 s;
    for (uint32_t i = 0; i < k_n; ++i)
        s = s.insert(i * 3u + 1u);
    EXPECT_EQ(std::distance(s.begin(), s.end()), static_cast<ptrdiff_t>(k_n));
}

TEST_F(IncrementalSetTest, IteratorDistanceAfterEraseDecreasesByOne) {
    const set_u32 full = set_u32{}.insert(1u).insert(2u).insert(3u).insert(4u);
    const set_u32 after = full.erase(2u);
    EXPECT_EQ(std::distance(after.begin(), after.end()), 3);
}

// ---------------------------------------------------------------------------
// Erase absent key: iteration unaffected
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, EraseAbsentKeyPreservesIterationOrder) {
    const set_u32 s   = set_u32{}.insert(10u).insert(20u).insert(30u);
    const set_u32 r   = s.erase(99u);
    EXPECT_EQ(collect(r), (std::vector<uint32_t>{10u, 20u, 30u}));
}

TEST_F(IncrementalSetTest, ChainedEraseAbsentKeysPreservesContents) {
    const set_u32 s = set_u32{}.insert(5u).insert(10u).insert(15u);
    const set_u32 r = s.erase(1u).erase(7u).erase(100u);
    EXPECT_EQ(collect(r), (std::vector<uint32_t>{5u, 10u, 15u}));
}

// ---------------------------------------------------------------------------
// Erase two-child node: treap_merge exercised
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, EraseTwoChildInteriorNodeIteratesCorrectly) {
    // Insert enough keys that an interior node with two children exists,
    // then erase it and verify sorted iteration.
    const set_u32 s = set_u32{}.insert(4u).insert(2u).insert(6u)
                                .insert(1u).insert(3u).insert(5u).insert(7u);
    const set_u32 r = s.erase(4u);
    EXPECT_EQ(collect(r), (std::vector<uint32_t>{1u, 2u, 3u, 5u, 6u, 7u}));
}

TEST_F(IncrementalSetTest, EraseAllTwoChildNodesIteratesCorrectly) {
    set_u32 s;
    for (uint32_t i = 1u; i <= 7u; ++i)
        s = s.insert(i);
    // erase all interior candidates
    for (uint32_t i : {4u, 2u, 6u})
        s = s.erase(i);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{1u, 3u, 5u, 7u}));
}

// ---------------------------------------------------------------------------
// Duplicate insert: iteration count stays at one
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, DuplicateInsertIterationYieldsOneElement) {
    set_u32 s = set_u32{}.insert(42u);
    for (int i = 0; i < 10; ++i)
        s = s.insert(42u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{42u}));
}

TEST_F(IncrementalSetTest, DuplicateInsertAmongOthersIterationUnchanged) {
    const set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u)
                                .insert(2u).insert(1u).insert(3u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{1u, 2u, 3u}));
}

// ---------------------------------------------------------------------------
// Insertion order invariance: same keys → same iteration
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, AscendingAndDescendingInsertYieldSameIteration) {
    set_u32 asc;
    for (uint32_t i = 1u; i <= 8u; ++i) asc = asc.insert(i);

    set_u32 desc;
    for (uint32_t i = 8u; i >= 1u; --i) desc = desc.insert(i);

    EXPECT_EQ(collect(asc), collect(desc));
}

TEST_F(IncrementalSetTest, ShuffledInsertYieldsSameSortedIterationAsAscending) {
    constexpr uint32_t k_n = 12u;
    set_u32 asc;
    for (uint32_t i = 0; i < k_n; ++i) asc = asc.insert(i);

    std::vector<uint32_t> keys(k_n);
    for (uint32_t i = 0; i < k_n; ++i) keys[i] = i;
    std::mt19937 rng(54321u);
    std::shuffle(keys.begin(), keys.end(), rng);

    set_u32 shuffled;
    for (uint32_t k : keys) shuffled = shuffled.insert(k);

    EXPECT_EQ(collect(asc), collect(shuffled));
}

// ---------------------------------------------------------------------------
// Erase in iteration order and reverse iteration order
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, EraseAllInAscendingOrderIterationEmptiesSet) {
    constexpr uint32_t k_n = 8u;
    set_u32 s;
    for (uint32_t i = 0; i < k_n; ++i) s = s.insert(i);

    for (uint32_t i = 0; i < k_n; ++i) {
        s = s.erase(i);
        const std::vector<uint32_t> result = collect(s);
        ASSERT_EQ(result.size(), k_n - i - 1u);
        for (uint32_t j = 0; j < result.size(); ++j)
            EXPECT_EQ(result[j], i + 1u + j) << "step " << i << " pos " << j;
    }
}

TEST_F(IncrementalSetTest, EraseAllInDescendingOrderIterationEmptiesSet) {
    constexpr uint32_t k_n = 8u;
    set_u32 s;
    for (uint32_t i = 0; i < k_n; ++i) s = s.insert(i);

    for (uint32_t i = k_n; i-- > 0;) {
        s = s.erase(i);
        const std::vector<uint32_t> result = collect(s);
        ASSERT_EQ(result.size(), i);
        for (uint32_t j = 0; j < i; ++j)
            EXPECT_EQ(result[j], j) << "step erasing " << i << " pos " << j;
    }
}

// ---------------------------------------------------------------------------
// Parent iteration stable after branching and modifying copies
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, ParentIterationUnchangedAfterBranchModified) {
    const set_u32 parent = set_u32{}.insert(3u).insert(7u).insert(11u);
    const std::vector<uint32_t> before = collect(parent);

    set_u32 branch = parent;
    for (uint32_t i = 0; i < 50u; ++i) branch = branch.insert(i * 2u);
    branch = branch.erase(3u).erase(7u).erase(11u);

    EXPECT_EQ(collect(parent), before);
}

// ---------------------------------------------------------------------------
// Alternating insert/erase of same key: final state reflected in iteration
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, AlternatingInsertEraseEndingWithInsert) {
    set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u);
    for (int i = 0; i < 5; ++i) {
        s = s.erase(2u);
        s = s.insert(2u);
    }
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{1u, 2u, 3u}));
}

TEST_F(IncrementalSetTest, AlternatingInsertEraseEndingWithErase) {
    set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u);
    for (int i = 0; i < 5; ++i) {
        s = s.insert(2u);
        s = s.erase(2u);
    }
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{1u, 3u}));
}

// ---------------------------------------------------------------------------
// Key 0 with priority 0 (degenerate priority case)
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, KeyZeroIteratesCorrectlyAmongLargeKeys) {
    const set_u32 s = set_u32{}.insert(100u).insert(200u).insert(0u).insert(50u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{0u, 50u, 100u, 200u}));
}

TEST_F(IncrementalSetTest, KeyZeroErasedIterationSkipsZero) {
    set_u32 s;
    for (uint32_t i = 0; i <= 5u; ++i) s = s.insert(i);
    s = s.erase(0u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{1u, 2u, 3u, 4u, 5u}));
}

// ---------------------------------------------------------------------------
// Deep branching: all leaves iterate independently
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, DeepBranchingAllSetsIterateCorrectly) {
    const set_u32 root    = set_u32{}.insert(5u).insert(10u).insert(15u);
    const set_u32 child_a = root.insert(1u);
    const set_u32 child_b = root.erase(10u);
    const set_u32 leaf_a1 = child_a.insert(20u);
    const set_u32 leaf_a2 = child_a.erase(5u);

    EXPECT_EQ(collect(root),    (std::vector<uint32_t>{5u, 10u, 15u}));
    EXPECT_EQ(collect(child_a), (std::vector<uint32_t>{1u, 5u, 10u, 15u}));
    EXPECT_EQ(collect(child_b), (std::vector<uint32_t>{5u, 15u}));
    EXPECT_EQ(collect(leaf_a1), (std::vector<uint32_t>{1u, 5u, 10u, 15u, 20u}));
    EXPECT_EQ(collect(leaf_a2), (std::vector<uint32_t>{1u, 10u, 15u}));
}

// ---------------------------------------------------------------------------
// Iterator stress: oracle comparison against std::set
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, IteratorStressRandomOpsMatchStdSetOrder) {
    constexpr int k_ops            = 2000;
    constexpr uint32_t k_key_range = 200u;
    constexpr uint32_t k_seed      = 77777u;

    std::mt19937 rng(k_seed);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_key_range - 1u);
    std::uniform_int_distribution<int>      pick_op(0, 1);

    set_u32 s;
    std::set<uint32_t> oracle;

    for (int step = 0; step < k_ops; ++step) {
        const uint32_t key = pick_key(rng);
        if (pick_op(rng) == 0) {
            s = s.insert(key);
            oracle.insert(key);
        } else {
            s = s.erase(key);
            oracle.erase(key);
        }

        const std::vector<uint32_t> result(oracle.begin(), oracle.end());
        ASSERT_EQ(collect(s), result) << "mismatch at step " << step;
    }
}

// ---------------------------------------------------------------------------
// Two independent iterators from the same set
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, TwoIteratorsFromSameBeginAreIndependent) {
    const set_u32 s = set_u32{}.insert(1u).insert(2u).insert(3u);
    auto it1 = s.begin();
    auto it2 = s.begin();

    EXPECT_EQ(*it1, 1u);
    EXPECT_EQ(*it2, 1u);
    ++it1;
    EXPECT_EQ(*it1, 2u);
    EXPECT_EQ(*it2, 1u); // it2 unmoved
    ++it1;
    ++it2;
    EXPECT_EQ(*it1, 3u);
    EXPECT_EQ(*it2, 2u);
}

TEST_F(IncrementalSetTest, TwoIteratorsWalkFullSetIndependently) {
    const set_u32 s = set_u32{}.insert(10u).insert(20u).insert(30u).insert(40u);
    std::vector<uint32_t> run1;
    std::vector<uint32_t> run2;

    auto it = s.begin();
    while (it != s.end()) { run1.push_back(*it); ++it; }

    for (uint32_t key : s) run2.push_back(key);

    EXPECT_EQ(run1, run2);
    EXPECT_EQ(run1, (std::vector<uint32_t>{10u, 20u, 30u, 40u}));
}

// ---------------------------------------------------------------------------
// Copy constructor preserves iteration
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, CopiedSetIteratesIdenticallyToOriginal) {
    const set_u32 original = set_u32{}.insert(5u).insert(2u).insert(8u).insert(1u);
    const set_u32 copy = original;
    EXPECT_EQ(collect(copy), collect(original));
}

TEST_F(IncrementalSetTest, CopyAndOriginalIterateCorrectlyAfterIndependentMutation) {
    const set_u32 base = set_u32{}.insert(10u).insert(20u).insert(30u);
    set_u32 a = base;
    set_u32 b = base;
    a = a.insert(5u).erase(20u);
    b = b.insert(40u).erase(10u);

    EXPECT_EQ(collect(a), (std::vector<uint32_t>{5u, 10u, 30u}));
    EXPECT_EQ(collect(b), (std::vector<uint32_t>{20u, 30u, 40u}));
    EXPECT_EQ(collect(base), (std::vector<uint32_t>{10u, 20u, 30u}));
}

// ---------------------------------------------------------------------------
// Erase absent from single-element set: iteration
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, EraseAbsentFromSingleElementSetIteratesOriginalElement) {
    const set_u32 s = set_u32{}.insert(42u).erase(99u);
    EXPECT_EQ(collect(s), (std::vector<uint32_t>{42u}));
}

TEST_F(IncrementalSetTest, DoubleEraseKeyThenIterationIsEmpty) {
    const set_u32 s = set_u32{}.insert(42u).erase(42u).erase(42u);
    EXPECT_TRUE(collect(s).empty());
    EXPECT_EQ(s.begin(), s.end());
}

// ---------------------------------------------------------------------------
// Keys near UINT32_MAX in sorted iteration
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, KeysNearMaxUint32IteratedInSortedOrder) {
    constexpr uint32_t k_max = 0xffffffffu;
    const set_u32 s = set_u32{}
        .insert(k_max).insert(k_max - 1u).insert(k_max - 2u)
        .insert(k_max - 3u).insert(k_max - 4u);
    const std::vector<uint32_t> expected{
        k_max - 4u, k_max - 3u, k_max - 2u, k_max - 1u, k_max};
    EXPECT_EQ(collect(s), expected);
}

TEST_F(IncrementalSetTest, MixedSmallAndLargeKeysIteratedInSortedOrder) {
    const set_u32 s = set_u32{}
        .insert(0u).insert(0xffffffffu)
        .insert(1u).insert(0xfffffffeu)
        .insert(2u).insert(0xfffffffdu);
    const std::vector<uint32_t> expected{
        0u, 1u, 2u, 0xfffffffdu, 0xfffffffeu, 0xffffffffu};
    EXPECT_EQ(collect(s), expected);
}

// ---------------------------------------------------------------------------
// STL algorithms work via const_iterator
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, StdFindLocatesExistingKey) {
    const set_u32 s = set_u32{}.insert(3u).insert(7u).insert(11u);
    const auto found = std::find(s.begin(), s.end(), 7u);
    ASSERT_NE(found, s.end());
    EXPECT_EQ(*found, 7u);
}

TEST_F(IncrementalSetTest, StdFindReturnsEndForAbsentKey) {
    const set_u32 s = set_u32{}.insert(3u).insert(7u).insert(11u);
    EXPECT_EQ(std::find(s.begin(), s.end(), 99u), s.end());
}

TEST_F(IncrementalSetTest, StdCountMatchesMembership) {
    const set_u32 s = set_u32{}.insert(3u).insert(7u).insert(11u);
    EXPECT_EQ(std::count(s.begin(), s.end(), 7u), 1);
    EXPECT_EQ(std::count(s.begin(), s.end(), 99u), 0);
}

// ---------------------------------------------------------------------------
// Iteration and contains agree on every element
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, IterationAndContainsAgreeOnEveryElement) {
    constexpr uint32_t k_key_range = 50u;
    set_u32 s;
    std::set<uint32_t> oracle;

    std::mt19937 rng(11111u);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_key_range - 1u);
    for (int i = 0; i < 200; ++i) {
        const uint32_t key = pick_key(rng);
        s = s.insert(key);
        oracle.insert(key);
    }

    // every key from iteration must pass contains
    for (uint32_t key : s)
        EXPECT_TRUE(s.contains(key)) << "key " << key << " in iteration but not contains";

    // every key in range must agree between oracle and contains
    for (uint32_t k = 0; k < k_key_range; ++k)
        EXPECT_EQ(s.contains(k), oracle.count(k) > 0) << "key " << k;
}

// ---------------------------------------------------------------------------
// Empty set via various paths
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, EraseAbsentFromEmptySetIterationStaysEmpty) {
    const set_u32 s = set_u32{}.erase(5u).erase(10u).erase(0u);
    EXPECT_EQ(s.begin(), s.end());
    EXPECT_TRUE(collect(s).empty());
}

TEST_F(IncrementalSetTest, FullRoundTripInsertAllEraseAllIterationEmpty) {
    constexpr uint32_t k_n = 20u;
    set_u32 s;
    for (uint32_t i = 0; i < k_n; ++i) s = s.insert(i);
    for (uint32_t i = 0; i < k_n; ++i) s = s.erase(i);
    EXPECT_EQ(s.begin(), s.end());
    EXPECT_TRUE(collect(s).empty());
}

// ---------------------------------------------------------------------------
// Linear version chain: all 50 sets live and iterable
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, LinearVersionChainAllSetsIterateCorrectly) {
    constexpr int k_versions = 50;
    constexpr uint32_t k_key_range = 30u;

    std::mt19937 rng(22222u);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_key_range - 1u);
    std::uniform_int_distribution<int>      pick_op(0, 1);

    std::vector<set_u32>           versions;
    std::vector<std::set<uint32_t>> oracles;
    versions.reserve(k_versions);
    oracles.reserve(k_versions);

    set_u32 s;
    std::set<uint32_t> oracle;
    for (int v = 0; v < k_versions; ++v) {
        const uint32_t key = pick_key(rng);
        if (pick_op(rng) == 0) { s = s.insert(key); oracle.insert(key); }
        else                   { s = s.erase(key);  oracle.erase(key);  }
        versions.push_back(s);
        oracles.push_back(oracle);
    }

    for (int v = 0; v < k_versions; ++v) {
        const std::vector<uint32_t> expected(oracles[v].begin(), oracles[v].end());
        ASSERT_EQ(collect(versions[v]), expected) << "version " << v;
    }
}

// ---------------------------------------------------------------------------
// Stress: large key set (500 keys) full iteration oracle
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, StressLargeSetIterationMatchesStdSet) {
    constexpr uint32_t k_n    = 500u;
    constexpr uint32_t k_seed = 33333u;

    std::mt19937 rng(k_seed);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_n * 2u - 1u);

    set_u32 s;
    std::set<uint32_t> oracle;
    for (uint32_t i = 0; i < k_n; ++i) {
        const uint32_t key = pick_key(rng);
        s = s.insert(key);
        oracle.insert(key);
    }

    const std::vector<uint32_t> expected(oracle.begin(), oracle.end());
    EXPECT_EQ(collect(s), expected);
}

// ---------------------------------------------------------------------------
// Stress: 20 branches from one parent, each with random mutations
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, StressManyBranchesIterationCorrect) {
    constexpr int k_branches   = 20;
    constexpr int k_ops        = 50;
    constexpr uint32_t k_key_range = 100u;
    constexpr uint32_t k_seed  = 44444u;

    std::mt19937 rng(k_seed);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_key_range - 1u);
    std::uniform_int_distribution<int>      pick_op(0, 1);

    // build a shared base
    set_u32 base;
    std::set<uint32_t> base_oracle;
    for (int i = 0; i < 30; ++i) {
        const uint32_t key = pick_key(rng);
        base = base.insert(key);
        base_oracle.insert(key);
    }

    std::vector<set_u32>           branches(k_branches, base);
    std::vector<std::set<uint32_t>> branch_oracles(k_branches, base_oracle);

    for (int b = 0; b < k_branches; ++b) {
        for (int op = 0; op < k_ops; ++op) {
            const uint32_t key = pick_key(rng);
            if (pick_op(rng) == 0) {
                branches[b] = branches[b].insert(key);
                branch_oracles[b].insert(key);
            } else {
                branches[b] = branches[b].erase(key);
                branch_oracles[b].erase(key);
            }
        }
    }

    for (int b = 0; b < k_branches; ++b) {
        const std::vector<uint32_t> expected(
            branch_oracles[b].begin(), branch_oracles[b].end());
        ASSERT_EQ(collect(branches[b]), expected) << "branch " << b;
    }

    // base set must be unchanged
    const std::vector<uint32_t> base_expected(base_oracle.begin(), base_oracle.end());
    EXPECT_EQ(collect(base), base_expected);
}

// ---------------------------------------------------------------------------
// Stress: retained snapshots verified via iteration (not just contains)
// ---------------------------------------------------------------------------

TEST_F(IncrementalSetTest, StressRetainedSnapshotsIterationCorrect) {
    constexpr int k_ops        = 3000;
    constexpr uint32_t k_key_range = 300u;
    constexpr uint32_t k_seed  = 55555u;

    std::mt19937 rng(k_seed);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_key_range - 1u);
    std::uniform_int_distribution<int>      pick_op(0, 1);

    std::vector<set_u32>           snapshots;
    std::vector<std::set<uint32_t>> snapshot_oracles;

    set_u32 s;
    std::set<uint32_t> oracle;
    for (int step = 0; step < k_ops; ++step) {
        const uint32_t key = pick_key(rng);
        if (pick_op(rng) == 0) { s = s.insert(key); oracle.insert(key); }
        else                   { s = s.erase(key);  oracle.erase(key);  }
        if (step % 15 == 14) {
            snapshots.push_back(s);
            snapshot_oracles.push_back(oracle);
        }
    }

    for (size_t i = 0; i < snapshots.size(); ++i) {
        const std::vector<uint32_t> expected(
            snapshot_oracles[i].begin(), snapshot_oracles[i].end());
        ASSERT_EQ(collect(snapshots[i]), expected) << "snapshot " << i;
    }
}

// ---------------------------------------------------------------------------
// Degenerate insertion order: ascending 0..100 must stay O(log n) depth
// ---------------------------------------------------------------------------

// Without priority-based rotations, inserting keys in ascending order
// produces a right-spine tree of depth 101 (plain BST worst case).
// The treap must rotate keys with higher priorities toward the root so
// that contains() requires at most O(log n) comparisons regardless of
// insertion order.
//
// We bound each contains() call to at most 4*log2(n)+2 comparisons.
// A degenerate tree would need up to 2*n comparisons for the deepest key;
// a balanced treap needs ~2*log2(n).

TEST_F(IncrementalSetTest, AscendingInsertContainsInLogComparisons) {
    using set_c = incremental_set<cmp_key>;
    constexpr int k_n = 1'000'000;

    set_c s;
    for (int i = 0; i < k_n; ++i) {
        g_cmp_count = 0;
        s = s.insert({static_cast<uint32_t>(i)});
    }

    // log2(1M) ≈ 20. Bound: 4 * log2(n) + 2 ≈ 82.
    // A degenerate right-spine tree (no rotations) would need ~2M comparisons
    // for the shallowest key and ~2 for the deepest — average ~1M. Bound is ~0.000082M.
    const int comparison_bound = static_cast<int>(4.0 * std::log2(k_n)) + 2;

    // Sample 1000 evenly-spaced keys — no need to check all 1M.
    const int sample_step = k_n / 1000;
    for (int i = 0; i < k_n; i += sample_step) {
        g_cmp_count = 0;
        EXPECT_TRUE(s.contains({static_cast<uint32_t>(i)}));
        EXPECT_LE(g_cmp_count, comparison_bound)
            << "key " << i << " took " << g_cmp_count
            << " comparisons (bound " << comparison_bound
            << "); degenerate tree would need ~" << 2 * (i + 1);
    }
}

TEST_F(IncrementalSetTest, AscendingInsertSameDepthAsShuffledInsert) {
    using set_c = incremental_set<cmp_key>;
    constexpr int k_n = 1'000'000;

    set_c asc;
    for (int i = 0; i < k_n; ++i) asc = asc.insert({static_cast<uint32_t>(i)});

    std::vector<uint32_t> keys(k_n);
    for (int i = 0; i < k_n; ++i) keys[i] = static_cast<uint32_t>(i);
    std::mt19937 rng(99999u);
    std::shuffle(keys.begin(), keys.end(), rng);
    set_c shuffled;
    for (uint32_t k : keys) shuffled = shuffled.insert({k});

    // Sample 1000 lookups. Because the treap shape depends only on key
    // priorities (hash of value, not insertion order), both sets must produce
    // identical comparison counts for every lookup.
    const int sample_step = k_n / 1000;
    int asc_total = 0;
    int shuffled_total = 0;
    for (int i = 0; i < k_n; i += sample_step) {
        g_cmp_count = 0; asc.contains({static_cast<uint32_t>(i)});      asc_total += g_cmp_count;
        g_cmp_count = 0; shuffled.contains({static_cast<uint32_t>(i)}); shuffled_total += g_cmp_count;
    }

    EXPECT_EQ(asc_total, shuffled_total)
        << "ascending total=" << asc_total << ", shuffled total=" << shuffled_total
        << " — must be equal since treap shape depends only on key priorities, not insertion order";
}
