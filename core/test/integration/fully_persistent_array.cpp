#include <gtest/gtest.h>
#include <deque>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>
#include "infrastructure/fully_persistent_array.hpp"
#include "infrastructure/order_maintenance.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

// ---------------------------------------------------------------------------
// Integration tests: real order_maintenance + real fully_persistent_array
//
// Unit tests for each component hand-craft or mock the other's
// responsibilities.  These tests verify that the two components compose
// correctly — in particular that om_label objects produced by
// order_maintenance work correctly as fully_persistent_array keys even after
// order_maintenance has performed internal relabeling.
// ---------------------------------------------------------------------------

namespace {

framed_expr make_framed(uint32_t functor_id) {
    // std::deque: push_back never invalidates existing references/pointers,
    // so framed_expr::skeleton pointers remain valid through all later calls.
    static std::deque<expr> exprs;
    exprs.push_back(expr{expr::functor{functor_id, {}}});
    return framed_expr{&exprs.back(), 0};
}

bool same_value(const std::optional<framed_expr>& result, const framed_expr& expected) {
    return result.has_value() && *result == expected;
}

} // namespace

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

struct FullyPersistentArrayIntegrationTest : public ::testing::Test {
    order_maintenance om_;
    fully_persistent_array bm_;

    const framed_expr val1_ = make_framed(101);
    const framed_expr val2_ = make_framed(102);
    const framed_expr val3_ = make_framed(103);
    const framed_expr val4_ = make_framed(104);
    const framed_expr val5_ = make_framed(105);

    constexpr static uint32_t k_var_x = 10;
    constexpr static uint32_t k_var_y = 11;
};

// ---------------------------------------------------------------------------
// Basic correctness with real OM labels
// ---------------------------------------------------------------------------

TEST_F(FullyPersistentArrayIntegrationTest, RealLabelsInheritanceWorks) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);

    bm_.record(root.open, root.close, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(child.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayIntegrationTest, RealLabelsSiblingIsolation) {
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);

    bm_.record(child_a.open, child_a.close, k_var_x, val2_);
    EXPECT_FALSE(bm_.query(child_b.open, k_var_x).has_value());
}

TEST_F(FullyPersistentArrayIntegrationTest, RealLabelsRebindingAndRestoration) {
    const om_interval root    = om_.allocate_root();
    const om_interval child   = om_.allocate_child_of(root);
    const om_interval sibling = om_.allocate_child_of(root);

    bm_.record(root.open,  root.close,  k_var_x, val1_);
    bm_.record(child.open, child.close, k_var_x, val2_);

    EXPECT_TRUE(same_value(bm_.query(child.open,   k_var_x), val2_));
    EXPECT_TRUE(same_value(bm_.query(sibling.open,  k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(root.open,     k_var_x), val1_));
}

TEST_F(FullyPersistentArrayIntegrationTest, RealLabelsGrandchildInheritance) {
    const om_interval root       = om_.allocate_root();
    const om_interval child      = om_.allocate_child_of(root);
    const om_interval grandchild = om_.allocate_child_of(child);

    bm_.record(root.open, root.close, k_var_x, val1_);
    EXPECT_TRUE(same_value(bm_.query(grandchild.open, k_var_x), val1_));
}

TEST_F(FullyPersistentArrayIntegrationTest, RealLabelsMultipleVariables) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);

    bm_.record(root.open,  root.close,  k_var_x, val1_);
    bm_.record(child.open, child.close, k_var_y, val2_);

    EXPECT_TRUE(same_value(bm_.query(child.open, k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(child.open, k_var_y), val2_));
    EXPECT_FALSE(bm_.query(root.open, k_var_y).has_value());
}

// ---------------------------------------------------------------------------
// Relabeling correctness end-to-end
//
// These tests allocate enough nodes to trigger relabeling inside
// order_maintenance, then verify that fully_persistent_array still returns
// correct results.  A stale-key bug (fully_persistent_array's std::map
// holding an old rank value while the om_label now reads a new rank) would
// surface here.
// ---------------------------------------------------------------------------

TEST_F(FullyPersistentArrayIntegrationTest, ManyChildrenThenQueryAll) {
    // Allocate 200 children (triggers relabeling), record a unique binding on
    // each, then verify each sees only its own value (not a sibling's).
    const om_interval root = om_.allocate_root();
    constexpr int k_count = 200;
    std::vector<om_interval> children;
    std::vector<framed_expr> values;
    children.reserve(k_count);
    values.reserve(k_count);

    for (int child_idx = 0; child_idx < k_count; ++child_idx) {
        children.push_back(om_.allocate_child_of(root));
        values.push_back(make_framed(static_cast<uint32_t>(200 + child_idx)));
        bm_.record(children[child_idx].open, children[child_idx].close,
                   k_var_x, values[child_idx]);
    }

    for (int child_idx = 0; child_idx < k_count; ++child_idx) {
        const auto result = bm_.query(children[child_idx].open, k_var_x);
        EXPECT_TRUE(same_value(result, values[child_idx]))
            << "child " << child_idx << " saw wrong value after relabeling";
    }
}

TEST_F(FullyPersistentArrayIntegrationTest, DeepChainRecordAndQuery) {
    // Allocate a 200-deep chain; record at root only; query at every depth.
    constexpr int k_depth = 200;
    std::vector<om_interval> levels;
    levels.reserve(k_depth);
    levels.push_back(om_.allocate_root());
    for (int depth = 1; depth < k_depth; ++depth)
        levels.push_back(om_.allocate_child_of(levels[depth - 1]));

    bm_.record(levels[0].open, levels[0].close, k_var_x, val1_);

    for (int depth = 0; depth < k_depth; ++depth) {
        EXPECT_TRUE(same_value(bm_.query(levels[depth].open, k_var_x), val1_))
            << "depth " << depth << " did not inherit root binding after relabeling";
    }
}

TEST_F(FullyPersistentArrayIntegrationTest, InterleavedAllocAndRecord) {
    // Alternate between allocating a new sibling and recording a binding on an
    // existing node.  Verifies that relabeling triggered mid-sequence does not
    // corrupt bindings recorded before the relabeling.
    const om_interval root = om_.allocate_root();
    bm_.record(root.open, root.close, k_var_x, val1_);

    constexpr int k_rounds = 100;
    std::vector<om_interval> children;
    std::vector<framed_expr> child_values;
    children.reserve(k_rounds);
    child_values.reserve(k_rounds);

    for (int round = 0; round < k_rounds; ++round) {
        children.push_back(om_.allocate_child_of(root));
        child_values.push_back(make_framed(static_cast<uint32_t>(300 + round)));
        bm_.record(children[round].open, children[round].close,
                   k_var_y, child_values[round]);
    }

    // After all allocation (which may have caused relabeling), verify:
    // - Root's binding of x is still visible everywhere.
    // - Each child's binding of y is correct and isolated.
    for (int round = 0; round < k_rounds; ++round) {
        EXPECT_TRUE(same_value(bm_.query(children[round].open, k_var_x), val1_))
            << "round " << round << ": root binding not visible after relabeling";
        EXPECT_TRUE(same_value(bm_.query(children[round].open, k_var_y), child_values[round]))
            << "round " << round << ": child's own binding wrong after relabeling";
    }

    // Verify sibling isolation: each child's y-binding is not visible to others.
    for (int round = 0; round < k_rounds; ++round) {
        for (int other = 0; other < k_rounds; ++other) {
            if (other == round) continue;
            EXPECT_FALSE(
                same_value(bm_.query(children[other].open, k_var_y), child_values[round]))
                << "child " << other << " saw child " << round << "'s y binding";
        }
    }
}

TEST_F(FullyPersistentArrayIntegrationTest, RelabelingDoesNotCorruptPriorRecords) {
    // Record bindings on a small tree, then force relabeling by adding many
    // siblings, then re-verify all prior bindings are still correct.
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);

    bm_.record(root.open,    root.close,    k_var_x, val1_);
    bm_.record(child_a.open, child_a.close, k_var_x, val2_);
    bm_.record(child_b.open, child_b.close, k_var_x, val3_);

    // Force relabeling.
    for (int sibling_idx = 0; sibling_idx < 200; ++sibling_idx)
        om_.allocate_child_of(root);

    // All pre-relabeling bindings must still be correct.
    EXPECT_TRUE(same_value(bm_.query(root.open,    k_var_x), val1_));
    EXPECT_TRUE(same_value(bm_.query(child_a.open, k_var_x), val2_));
    EXPECT_TRUE(same_value(bm_.query(child_b.open, k_var_x), val3_));
}

// ---------------------------------------------------------------------------
// Two fully_persistent_array instances sharing one order_maintenance: records
// in one must not appear when querying the other.
// This mirrors the PUD usage where there is one base bind map plus one per-goal
// bind map, all sharing the same om_interval labels.
// ---------------------------------------------------------------------------

TEST_F(FullyPersistentArrayIntegrationTest, TwoFullyPersistentArraysShareOrderMaintenance) {
    fully_persistent_array bm2_;  // second array, same om_

    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);

    // Record x in bm_ and y in bm2_ (using the same interval labels).
    bm_.record(root.open, root.close, k_var_x, val1_);
    bm2_.record(root.open, root.close, k_var_y, val2_);

    // bm_ sees x but not y.
    EXPECT_TRUE( same_value(bm_.query(child.open, k_var_x), val1_));
    EXPECT_FALSE(bm_.query(child.open, k_var_y).has_value());

    // bm2_ sees y but not x.
    EXPECT_TRUE( same_value(bm2_.query(child.open, k_var_y), val2_));
    EXPECT_FALSE(bm2_.query(child.open, k_var_x).has_value());
}

// ---------------------------------------------------------------------------
// After relabeling in one root's subtree, queries in the other root's subtree
// must still return correct results — the stale-key bug would manifest here
// if relabeling changes ranks that fully_persistent_array has stored as map keys.
// ---------------------------------------------------------------------------

TEST_F(FullyPersistentArrayIntegrationTest, CrossSubtreeIsolationAfterRelabeling) {
    const om_interval root_a = om_.allocate_root();
    const om_interval root_b = om_.allocate_root();

    bm_.record(root_b.open, root_b.close, k_var_x, val3_);

    // Force relabeling inside root_a.
    constexpr int k_count = 200;
    for (int grand_idx = 0; grand_idx < k_count; ++grand_idx)
        om_.allocate_child_of(root_a);

    // Query inside root_b must still see val3 despite relabeling in root_a.
    const om_interval child_b = om_.allocate_child_of(root_b);
    EXPECT_TRUE(same_value(bm_.query(child_b.open, k_var_x), val3_))
        << "root_b's binding corrupted by relabeling in root_a's subtree";
}

// ---------------------------------------------------------------------------
// Deep chain where each level rebinds the same variable.
// After relabeling, each level must see its own binding and the sibling of
// the chain's first node must see only the root's binding.
// Catch bug: relabeling shuffles ranks in a way that predecessor searches
// in fully_persistent_array now find the wrong timeline entry.
// ---------------------------------------------------------------------------

TEST_F(FullyPersistentArrayIntegrationTest, DeepChainEachLevelRebindsSameVar) {
    constexpr int k_depth = 50;
    std::vector<om_interval> levels;
    std::vector<framed_expr> level_values;
    levels.reserve(k_depth + 1);
    level_values.reserve(k_depth + 1);

    levels.push_back(om_.allocate_root());
    level_values.push_back(make_framed(500));
    bm_.record(levels[0].open, levels[0].close, k_var_x, level_values[0]);

    for (int depth = 1; depth <= k_depth; ++depth) {
        levels.push_back(om_.allocate_child_of(levels[depth - 1]));
        level_values.push_back(make_framed(static_cast<uint32_t>(500 + depth)));
        bm_.record(levels[depth].open, levels[depth].close,
                   k_var_x, level_values[depth]);
    }

    // Each level must see its own binding.
    for (int depth = 0; depth <= k_depth; ++depth) {
        EXPECT_TRUE(same_value(bm_.query(levels[depth].open, k_var_x), level_values[depth]))
            << "depth " << depth << " sees wrong binding";
    }

    // A sibling of the first node (depth 1) must see the root's binding.
    const om_interval sibling = om_.allocate_child_of(levels[0]);
    EXPECT_TRUE(same_value(bm_.query(sibling.open, k_var_x), level_values[0]))
        << "sibling of depth-1 node does not see root binding";
}

// ---------------------------------------------------------------------------
// Interleaved allocations from two independent roots: alternate between
// adding children to root_a and root_b.  Verify that all recorded bindings
// remain correct across the interleaving (which may cause relabeling that
// could corrupt the other root's labels).
// ---------------------------------------------------------------------------

TEST_F(FullyPersistentArrayIntegrationTest, InterleavedAllocationsFromTwoRoots) {
    const om_interval root_a = om_.allocate_root();
    const om_interval root_b = om_.allocate_root();

    bm_.record(root_a.open, root_a.close, k_var_x, val1_);
    bm_.record(root_b.open, root_b.close, k_var_y, val2_);

    constexpr int k_rounds = 50;
    std::vector<om_interval> children_a;
    std::vector<om_interval> children_b;
    children_a.reserve(k_rounds);
    children_b.reserve(k_rounds);

    for (int round = 0; round < k_rounds; ++round) {
        children_a.push_back(om_.allocate_child_of(root_a));
        children_b.push_back(om_.allocate_child_of(root_b));
    }

    // All of root_a's children must inherit x and not see y.
    for (int round = 0; round < k_rounds; ++round) {
        EXPECT_TRUE(same_value(bm_.query(children_a[round].open, k_var_x), val1_))
            << "root_a child[" << round << "] lost x binding";
        EXPECT_FALSE(bm_.query(children_a[round].open, k_var_y).has_value())
            << "root_a child[" << round << "] incorrectly sees root_b's y binding";
    }

    // All of root_b's children must inherit y and not see x.
    for (int round = 0; round < k_rounds; ++round) {
        EXPECT_TRUE(same_value(bm_.query(children_b[round].open, k_var_y), val2_))
            << "root_b child[" << round << "] lost y binding";
        EXPECT_FALSE(bm_.query(children_b[round].open, k_var_x).has_value())
            << "root_b child[" << round << "] incorrectly sees root_a's x binding";
    }
}

// ---------------------------------------------------------------------------
// RNG-based end-to-end property test (seed 42).
//
// Builds a random 100-node tree using real order_maintenance, records random
// bindings for 5 variables in a real fully_persistent_array, then for every
// (node, var) pair verifies the query result against an expected value
// computed by walking up the parent chain — the simplest possible correct
// implementation.
//
// This is the most comprehensive single test: it simultaneously checks OM
// label validity, fully_persistent_array predecessor logic, close-event
// restoration, sibling isolation, and multi-variable independence, all under
// arbitrary tree shapes and relabeling triggers.
// ---------------------------------------------------------------------------

TEST_F(FullyPersistentArrayIntegrationTest, RandomTreeRandomBindingsEndToEnd) {
    std::mt19937 rng(42);

    constexpr int k_node_count = 100;
    constexpr int k_var_count  = 5;

    std::vector<om_interval> intervals;
    std::vector<int>         parent_idx;
    intervals.reserve(k_node_count);
    parent_idx.reserve(k_node_count);

    // Build the random tree.
    intervals.push_back(om_.allocate_root());
    parent_idx.push_back(-1);
    for (int idx = 1; idx < k_node_count; ++idx) {
        const bool make_root = (rng() % 7 == 0);
        if (make_root) {
            intervals.push_back(om_.allocate_root());
            parent_idx.push_back(-1);
        } else {
            const int parent = static_cast<int>(rng() % static_cast<uint32_t>(idx));
            intervals.push_back(om_.allocate_child_of(intervals[parent]));
            parent_idx.push_back(parent);
        }
    }

    // Decide randomly which (node, var) pairs get a binding, and with what value.
    // bindings[node_idx][var_id] = framed_expr
    std::vector<std::unordered_map<uint32_t, framed_expr>> bindings(k_node_count);
    std::deque<expr> expr_store;  // stable storage for expr* pointers
    uint32_t next_functor_id = 1000;

    for (int node_idx = 0; node_idx < k_node_count; ++node_idx) {
        for (int var_idx = 0; var_idx < k_var_count; ++var_idx) {
            if (rng() % 3 == 0) {  // ~33% chance of binding each (node, var)
                expr_store.push_back(expr{expr::functor{next_functor_id++, {}}});
                const framed_expr fe{&expr_store.back(), 0};
                bindings[node_idx][static_cast<uint32_t>(var_idx)] = fe;
                bm_.record(intervals[node_idx].open,
                           intervals[node_idx].close,
                           static_cast<uint32_t>(var_idx), fe);
            }
        }
    }

    // Compute expected value for (node_idx, var_id) by walking up the parent chain.
    auto expected_value = [&](int node_idx, uint32_t var_id)
        -> std::optional<framed_expr> {
        int cur = node_idx;
        while (cur != -1) {
            auto it = bindings[cur].find(var_id);
            if (it != bindings[cur].end())
                return it->second;
            cur = parent_idx[cur];
        }
        return std::nullopt;
    };

    // Verify every (node, var) pair.
    for (int node_idx = 0; node_idx < k_node_count; ++node_idx) {
        for (int var_idx = 0; var_idx < k_var_count; ++var_idx) {
            const uint32_t var_id = static_cast<uint32_t>(var_idx);
            const auto expected   = expected_value(node_idx, var_id);
            const auto actual     = bm_.query(intervals[node_idx].open, var_id);
            EXPECT_EQ(actual.has_value(), expected.has_value())
                << "node " << node_idx << " var " << var_idx
                << ": has_value mismatch";
            if (actual.has_value() && expected.has_value()) {
                EXPECT_EQ(*actual, *expected)
                    << "node " << node_idx << " var " << var_idx
                    << ": value mismatch";
            }
        }
    }
}

// Second RNG seed — different tree topology and binding density.
TEST_F(FullyPersistentArrayIntegrationTest, RandomTreeAlternateSeedEndToEnd) {
    std::mt19937 rng(271828);

    constexpr int k_node_count = 120;
    constexpr int k_var_count  = 6;

    std::vector<om_interval> intervals;
    std::vector<int>         parent_idx;
    intervals.reserve(k_node_count);
    parent_idx.reserve(k_node_count);

    intervals.push_back(om_.allocate_root());
    parent_idx.push_back(-1);
    for (int idx = 1; idx < k_node_count; ++idx) {
        const bool make_root = (rng() % 10 == 0);
        if (make_root) {
            intervals.push_back(om_.allocate_root());
            parent_idx.push_back(-1);
        } else {
            const int parent = static_cast<int>(rng() % static_cast<uint32_t>(idx));
            intervals.push_back(om_.allocate_child_of(intervals[parent]));
            parent_idx.push_back(parent);
        }
    }

    std::vector<std::unordered_map<uint32_t, framed_expr>> bindings(k_node_count);
    std::deque<expr> expr_store;
    uint32_t next_functor_id = 2000;

    for (int node_idx = 0; node_idx < k_node_count; ++node_idx) {
        for (int var_idx = 0; var_idx < k_var_count; ++var_idx) {
            if (rng() % 4 == 0) {
                expr_store.push_back(expr{expr::functor{next_functor_id++, {}}});
                const framed_expr fe{&expr_store.back(), 0};
                bindings[node_idx][static_cast<uint32_t>(var_idx)] = fe;
                bm_.record(intervals[node_idx].open,
                           intervals[node_idx].close,
                           static_cast<uint32_t>(var_idx), fe);
            }
        }
    }

    auto expected_value = [&](int node_idx, uint32_t var_id)
        -> std::optional<framed_expr> {
        int cur = node_idx;
        while (cur != -1) {
            auto it = bindings[cur].find(var_id);
            if (it != bindings[cur].end()) return it->second;
            cur = parent_idx[cur];
        }
        return std::nullopt;
    };

    for (int node_idx = 0; node_idx < k_node_count; ++node_idx) {
        for (int var_idx = 0; var_idx < k_var_count; ++var_idx) {
            const uint32_t var_id = static_cast<uint32_t>(var_idx);
            const auto expected   = expected_value(node_idx, var_id);
            const auto actual     = bm_.query(intervals[node_idx].open, var_id);
            EXPECT_EQ(actual.has_value(), expected.has_value())
                << "node " << node_idx << " var " << var_idx << " (seed 271828)";
            if (actual.has_value() && expected.has_value())
                EXPECT_EQ(*actual, *expected)
                    << "node " << node_idx << " var " << var_idx << " (seed 271828)";
        }
    }
}

// ---------------------------------------------------------------------------
// Star tree (100 children) with unique bindings per child.
// Recorded before relabeling completes; after all relabeling, every child
// must see only its own binding and not any sibling's.
// ---------------------------------------------------------------------------

TEST_F(FullyPersistentArrayIntegrationTest, StarTreeWithManyUniqueBindings) {
    const om_interval root = om_.allocate_root();
    constexpr int k_count = 100;

    std::vector<om_interval> children;
    std::vector<framed_expr> child_bindings;
    children.reserve(k_count);
    child_bindings.reserve(k_count);

    for (int child_idx = 0; child_idx < k_count; ++child_idx) {
        children.push_back(om_.allocate_child_of(root));
        child_bindings.push_back(make_framed(static_cast<uint32_t>(3000 + child_idx)));
        bm_.record(children[child_idx].open, children[child_idx].close,
                   k_var_x, child_bindings[child_idx]);
    }

    // Root has no binding — every child sees only its own.
    for (int child_idx = 0; child_idx < k_count; ++child_idx) {
        const auto result = bm_.query(children[child_idx].open, k_var_x);
        EXPECT_TRUE(result.has_value())
            << "child " << child_idx << " has no binding";
        EXPECT_EQ(*result, child_bindings[child_idx])
            << "child " << child_idx << " sees wrong value (sibling contamination?)";
    }
}

// ---------------------------------------------------------------------------
// 500-deep chain in fully_persistent_array: each level rebinds the same
// variable.  After multiple relabeling rounds, every level must still see its
// own value, and a sibling added after the chain sees only the root's value.
// ---------------------------------------------------------------------------

TEST_F(FullyPersistentArrayIntegrationTest, VeryDeepChainMultipleRelabelings) {
    constexpr int k_depth = 500;
    std::vector<om_interval> levels;
    std::vector<framed_expr> level_values;
    levels.reserve(k_depth);
    level_values.reserve(k_depth);

    levels.push_back(om_.allocate_root());
    level_values.push_back(make_framed(4000));
    bm_.record(levels[0].open, levels[0].close, k_var_x, level_values[0]);

    for (int depth = 1; depth < k_depth; ++depth) {
        levels.push_back(om_.allocate_child_of(levels[depth - 1]));
        level_values.push_back(make_framed(static_cast<uint32_t>(4000 + depth)));
        bm_.record(levels[depth].open, levels[depth].close,
                   k_var_x, level_values[depth]);
    }

    // Every level must see its own value.
    for (int depth = 0; depth < k_depth; ++depth) {
        EXPECT_TRUE(same_value(bm_.query(levels[depth].open, k_var_x), level_values[depth]))
            << "depth " << depth << " sees wrong value after multiple relabelings";
    }

    // A sibling of depth-1 (i.e., another child of root) must see root's value.
    const om_interval sibling = om_.allocate_child_of(levels[0]);
    EXPECT_TRUE(same_value(bm_.query(sibling.open, k_var_x), level_values[0]))
        << "sibling of chain sees wrong value (should be root's binding)";
}
