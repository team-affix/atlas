#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <vector>
#include <immer/set.hpp>

namespace {

using iset = immer::set<uint32_t>;

std::unordered_set<uint32_t> collect(const iset& s) {
    std::unordered_set<uint32_t> result;
    for (uint32_t key : s)
        result.insert(key);
    return result;
}

} // namespace

struct ImmerSetTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// Default-constructed set
// ---------------------------------------------------------------------------

TEST_F(ImmerSetTest, DefaultSetIsEmpty) {
    iset s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
}

TEST_F(ImmerSetTest, DefaultSetCountIsZero) {
    iset s;
    EXPECT_EQ(s.count(0u), 0u);
    EXPECT_EQ(s.count(42u), 0u);
    EXPECT_EQ(s.count(0xffffffffu), 0u);
}

TEST_F(ImmerSetTest, DefaultSetFindReturnsNull) {
    iset s;
    EXPECT_EQ(s.find(42u), nullptr);
}

TEST_F(ImmerSetTest, DefaultSetBeginEqualsEnd) {
    iset s;
    EXPECT_EQ(s.begin(), s.end());
}

// ---------------------------------------------------------------------------
// Insert first key
// ---------------------------------------------------------------------------

TEST_F(ImmerSetTest, InsertFirstKeyCountIsOne) {
    iset s = iset{}.insert(7u);
    EXPECT_EQ(s.count(7u), 1u);
    EXPECT_EQ(s.count(0u), 0u);
    EXPECT_EQ(s.size(), 1u);
}

TEST_F(ImmerSetTest, InsertFirstKeyFindReturnsPointer) {
    iset s = iset{}.insert(7u);
    const uint32_t* p = s.find(7u);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 7u);
}

TEST_F(ImmerSetTest, InsertFirstKeyReceiverStillEmpty) {
    iset empty;
    const iset s = empty.insert(7u);
    EXPECT_EQ(empty.size(), 0u);
    EXPECT_EQ(empty.count(7u), 0u);
    EXPECT_EQ(s.count(7u), 1u);
}

// ---------------------------------------------------------------------------
// Duplicate insert
// ---------------------------------------------------------------------------

TEST_F(ImmerSetTest, DuplicateInsertSizeStaysOne) {
    iset s = iset{}.insert(5u).insert(5u).insert(5u);
    EXPECT_EQ(s.size(), 1u);
    EXPECT_EQ(s.count(5u), 1u);
}

// ---------------------------------------------------------------------------
// Erase
// ---------------------------------------------------------------------------

TEST_F(ImmerSetTest, EraseOnlyElementYieldsEmpty) {
    const iset with_key = iset{}.insert(42u);
    const iset empty    = with_key.erase(42u);
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.count(42u), 0u);
    EXPECT_EQ(with_key.count(42u), 1u);
}

TEST_F(ImmerSetTest, EraseAbsentKeyLeavesContentsUnchanged) {
    const iset s = iset{}.insert(1u).insert(2u).insert(3u);
    const iset r = s.erase(99u);
    EXPECT_EQ(r.size(), 3u);
    EXPECT_EQ(r.count(1u), 1u);
    EXPECT_EQ(r.count(2u), 1u);
    EXPECT_EQ(r.count(3u), 1u);
    EXPECT_EQ(r.count(99u), 0u);
}

TEST_F(ImmerSetTest, EraseReceiverUnchanged) {
    const iset s = iset{}.insert(1u).insert(2u).insert(3u);
    const iset r = s.erase(2u);
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s.count(2u), 1u);
    EXPECT_EQ(r.count(2u), 0u);
}

TEST_F(ImmerSetTest, EraseFromEmptyDoesNotContainKey) {
    const iset r = iset{}.erase(42u);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(r.count(42u), 0u);
}

// ---------------------------------------------------------------------------
// Size
// ---------------------------------------------------------------------------

TEST_F(ImmerSetTest, SizeAfterInserts) {
    iset s;
    for (uint32_t i = 0; i < 10u; ++i) s = s.insert(i);
    EXPECT_EQ(s.size(), 10u);
}

TEST_F(ImmerSetTest, SizeAfterDuplicateInserts) {
    iset s;
    for (int i = 0; i < 5; ++i) s = s.insert(42u);
    EXPECT_EQ(s.size(), 1u);
}

// ---------------------------------------------------------------------------
// Branching (structural sharing)
// ---------------------------------------------------------------------------

TEST_F(ImmerSetTest, TwoBranchesFromParentAreIndependent) {
    const iset parent;
    const iset left  = parent.insert(1u);
    const iset right = parent.insert(2u);

    EXPECT_EQ(left.count(1u), 1u);
    EXPECT_EQ(left.count(2u), 0u);
    EXPECT_EQ(right.count(1u), 0u);
    EXPECT_EQ(right.count(2u), 1u);
    EXPECT_EQ(parent.size(), 0u);
}

TEST_F(ImmerSetTest, FurtherInsertOnBranchInvisibleToSibling) {
    const iset parent = iset{}.insert(10u);
    const iset b1 = parent.insert(20u);
    const iset b2 = parent.insert(30u);

    EXPECT_EQ(b1.count(10u), 1u); EXPECT_EQ(b1.count(20u), 1u);
    EXPECT_EQ(b1.count(30u), 0u);
    EXPECT_EQ(b2.count(10u), 1u); EXPECT_EQ(b2.count(30u), 1u);
    EXPECT_EQ(b2.count(20u), 0u);
    EXPECT_EQ(parent.size(), 1u);
}

// ---------------------------------------------------------------------------
// Iteration
// ---------------------------------------------------------------------------

TEST_F(ImmerSetTest, IterationContainsAllInsertedKeys) {
    constexpr uint32_t k_n = 20u;
    iset s;
    for (uint32_t i = 0; i < k_n; ++i) s = s.insert(i);
    const std::unordered_set<uint32_t> iterated = collect(s);
    EXPECT_EQ(iterated.size(), k_n);
    for (uint32_t i = 0; i < k_n; ++i)
        EXPECT_EQ(iterated.count(i), 1u) << "missing key " << i;
}

TEST_F(ImmerSetTest, IterationAfterEraseSkipsErasedKey) {
    const iset s = iset{}.insert(1u).insert(2u).insert(3u).erase(2u);
    const std::unordered_set<uint32_t> iterated = collect(s);
    EXPECT_EQ(iterated.size(), 2u);
    EXPECT_EQ(iterated.count(1u), 1u);
    EXPECT_EQ(iterated.count(2u), 0u);
    EXPECT_EQ(iterated.count(3u), 1u);
}

TEST_F(ImmerSetTest, IterationNoDuplicateElements) {
    iset s;
    for (int i = 0; i < 5; ++i) s = s.insert(42u);
    s = s.insert(1u).insert(2u);
    const std::unordered_set<uint32_t> iterated = collect(s);
    EXPECT_EQ(iterated.size(), s.size());
}

TEST_F(ImmerSetTest, TwoIndependentIterators) {
    const iset s = iset{}.insert(1u).insert(2u).insert(3u);
    auto it1 = s.begin();
    auto it2 = s.begin();
    EXPECT_EQ(it1, it2);
    ++it1;
    EXPECT_NE(it1, it2);
}

// ---------------------------------------------------------------------------
// Boundary keys
// ---------------------------------------------------------------------------

TEST_F(ImmerSetTest, KeyZeroAlone) {
    const iset s = iset{}.insert(0u);
    EXPECT_EQ(s.count(0u), 1u);
    EXPECT_EQ(s.count(1u), 0u);
    EXPECT_EQ(s.size(), 1u);
}

TEST_F(ImmerSetTest, KeyMaxUint32Alone) {
    const iset s = iset{}.insert(0xffffffffu);
    EXPECT_EQ(s.count(0xffffffffu), 1u);
    EXPECT_EQ(s.count(0u), 0u);
}

TEST_F(ImmerSetTest, KeyZeroAndKeyMaxTogether) {
    const iset s = iset{}.insert(0u).insert(0xffffffffu);
    EXPECT_EQ(s.size(), 2u);
    EXPECT_EQ(s.count(0u), 1u);
    EXPECT_EQ(s.count(0xffffffffu), 1u);
}

// ---------------------------------------------------------------------------
// Reinsert after erase
// ---------------------------------------------------------------------------

TEST_F(ImmerSetTest, ReinsertAfterErase) {
    const iset with_key  = iset{}.insert(7u);
    const iset without   = with_key.erase(7u);
    const iset reinserted = without.insert(7u);

    EXPECT_EQ(with_key.count(7u), 1u);
    EXPECT_EQ(without.count(7u), 0u);
    EXPECT_EQ(reinserted.count(7u), 1u);
}

// ---------------------------------------------------------------------------
// Stress: oracle comparison against std::unordered_set
// ---------------------------------------------------------------------------

TEST_F(ImmerSetTest, StressRandomOpsMatchUnorderedSetOracle) {
    constexpr int k_ops            = 2000;
    constexpr uint32_t k_key_range = 200u;
    constexpr uint32_t k_seed      = 13579u;

    std::mt19937 rng(k_seed);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_key_range - 1u);
    std::uniform_int_distribution<int>      pick_op(0, 1);

    iset s;
    std::unordered_set<uint32_t> oracle;

    for (int step = 0; step < k_ops; ++step) {
        const uint32_t key = pick_key(rng);
        if (pick_op(rng) == 0) { s = s.insert(key); oracle.insert(key); }
        else                   { s = s.erase(key);  oracle.erase(key);  }

        ASSERT_EQ(s.size(), oracle.size()) << "size mismatch at step " << step;
        for (uint32_t k = 0; k < k_key_range; ++k)
            ASSERT_EQ(s.count(k), oracle.count(k)) << "step " << step << " key " << k;
    }
}

TEST_F(ImmerSetTest, StressIterationMatchesOracleContents) {
    constexpr int k_ops            = 1000;
    constexpr uint32_t k_key_range = 100u;
    constexpr uint32_t k_seed      = 24680u;

    std::mt19937 rng(k_seed);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_key_range - 1u);
    std::uniform_int_distribution<int>      pick_op(0, 1);

    iset s;
    std::unordered_set<uint32_t> oracle;

    for (int step = 0; step < k_ops; ++step) {
        const uint32_t key = pick_key(rng);
        if (pick_op(rng) == 0) { s = s.insert(key); oracle.insert(key); }
        else                   { s = s.erase(key);  oracle.erase(key);  }

        const std::unordered_set<uint32_t> iterated = collect(s);
        ASSERT_EQ(iterated, oracle) << "iteration mismatch at step " << step;
    }
}

TEST_F(ImmerSetTest, StressMultipleLiveSetsMatchOracles) {
    constexpr int k_live_count  = 16;
    constexpr int k_ops         = 2000;
    constexpr uint32_t k_key_range = 200u;
    constexpr uint32_t k_seed   = 11223u;

    std::mt19937 rng(k_seed);
    std::uniform_int_distribution<int>      pick_set(0, k_live_count - 1);
    std::uniform_int_distribution<uint32_t> pick_key(0u, k_key_range - 1u);
    std::uniform_int_distribution<int>      pick_op(0, 2);

    std::vector<iset>                          live(k_live_count);
    std::vector<std::unordered_set<uint32_t>>  oracles(k_live_count);

    for (int step = 0; step < k_ops; ++step) {
        const int src = pick_set(rng);
        const int op  = pick_op(rng);
        const uint32_t key = pick_key(rng);

        if (op == 2) {
            const int dst = pick_set(rng);
            live[dst]    = live[src];
            oracles[dst] = oracles[src];
        } else if (op == 0) {
            live[src] = live[src].insert(key);
            oracles[src].insert(key);
        } else {
            live[src] = live[src].erase(key);
            oracles[src].erase(key);
        }

        for (int idx = 0; idx < k_live_count; ++idx)
            for (uint32_t k = 0; k < k_key_range; ++k)
                ASSERT_EQ(live[idx].count(k), oracles[idx].count(k))
                    << "step " << step << " set " << idx << " key " << k;
    }
}
