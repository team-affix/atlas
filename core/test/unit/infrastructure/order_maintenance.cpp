#include <gtest/gtest.h>
#include <random>
#include <vector>
#include "infrastructure/order_maintenance.hpp"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool is_ancestor(const om_interval& ancestor, const om_interval& descendant) {
    return ancestor.open < descendant.open && descendant.close < ancestor.close;
}

static bool intervals_non_overlapping(const om_interval& left, const om_interval& right) {
    // left entirely before right or right entirely before left
    return left.close < right.open || right.close < left.open;
}

static bool interval_valid(const om_interval& interval) {
    return interval.open < interval.close;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

struct OrderMaintenanceTest : public ::testing::Test {
    order_maintenance om_;
};

// ---------------------------------------------------------------------------
// Allocation basics
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, RootOpenIsLessThanRootClose) {
    const om_interval root = om_.allocate_root();
    EXPECT_TRUE(interval_valid(root));
}

TEST_F(OrderMaintenanceTest, TwoRootsAreNonOverlapping) {
    const om_interval root_a = om_.allocate_root();
    const om_interval root_b = om_.allocate_root();
    EXPECT_TRUE(intervals_non_overlapping(root_a, root_b));
    EXPECT_TRUE(interval_valid(root_a));
    EXPECT_TRUE(interval_valid(root_b));
}

TEST_F(OrderMaintenanceTest, ChildIntervalNestedInParent) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    EXPECT_TRUE(is_ancestor(root, child));
}

TEST_F(OrderMaintenanceTest, GrandchildNestedInChild) {
    const om_interval root       = om_.allocate_root();
    const om_interval child      = om_.allocate_child_of(root);
    const om_interval grandchild = om_.allocate_child_of(child);
    EXPECT_TRUE(is_ancestor(root, child));
    EXPECT_TRUE(is_ancestor(child, grandchild));
    EXPECT_TRUE(is_ancestor(root, grandchild));
}

TEST_F(OrderMaintenanceTest, GreatGrandchildNestedThreeLevels) {
    const om_interval root        = om_.allocate_root();
    const om_interval child       = om_.allocate_child_of(root);
    const om_interval grandchild  = om_.allocate_child_of(child);
    const om_interval great_grand = om_.allocate_child_of(grandchild);
    EXPECT_TRUE(is_ancestor(root, great_grand));
    EXPECT_TRUE(is_ancestor(child, great_grand));
    EXPECT_TRUE(is_ancestor(grandchild, great_grand));
    EXPECT_TRUE(interval_valid(great_grand));
}

TEST_F(OrderMaintenanceTest, TwoChildrenOfSameParentNonOverlapping) {
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);
    EXPECT_TRUE(intervals_non_overlapping(child_a, child_b));
}

TEST_F(OrderMaintenanceTest, TwoChildrenBothNestedInParent) {
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);
    EXPECT_TRUE(is_ancestor(root, child_a));
    EXPECT_TRUE(is_ancestor(root, child_b));
}

TEST_F(OrderMaintenanceTest, ThreeChildrenAllNestedAndNonOverlapping) {
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);
    const om_interval child_c = om_.allocate_child_of(root);
    EXPECT_TRUE(is_ancestor(root, child_a));
    EXPECT_TRUE(is_ancestor(root, child_b));
    EXPECT_TRUE(is_ancestor(root, child_c));
    EXPECT_TRUE(intervals_non_overlapping(child_a, child_b));
    EXPECT_TRUE(intervals_non_overlapping(child_b, child_c));
    EXPECT_TRUE(intervals_non_overlapping(child_a, child_c));
}

// ---------------------------------------------------------------------------
// Sibling ordering — later-allocated sibling opens after earlier sibling
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, SiblingsOrderedByAllocation) {
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);
    EXPECT_TRUE(child_a.open < child_b.open);
}

TEST_F(OrderMaintenanceTest, ThreeSiblingsFullyOrdered) {
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);
    const om_interval child_c = om_.allocate_child_of(root);
    EXPECT_TRUE(child_a.open < child_b.open);
    EXPECT_TRUE(child_b.open < child_c.open);
}

// ---------------------------------------------------------------------------
// Ancestor predicate
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, ParentIsAncestorOfChild) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    EXPECT_TRUE(is_ancestor(root, child));
}

TEST_F(OrderMaintenanceTest, ChildIsNotAncestorOfParent) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    EXPECT_FALSE(is_ancestor(child, root));
}

TEST_F(OrderMaintenanceTest, SiblingIsNotAncestorOfSibling) {
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);
    EXPECT_FALSE(is_ancestor(child_a, child_b));
    EXPECT_FALSE(is_ancestor(child_b, child_a));
}

TEST_F(OrderMaintenanceTest, GrandchildIsNotAncestorOfSibling) {
    const om_interval root       = om_.allocate_root();
    const om_interval child_a    = om_.allocate_child_of(root);
    const om_interval grandchild = om_.allocate_child_of(child_a);
    const om_interval child_b    = om_.allocate_child_of(root);
    EXPECT_FALSE(is_ancestor(grandchild, child_b));
    EXPECT_FALSE(is_ancestor(child_b, grandchild));
}

// ---------------------------------------------------------------------------
// Stress / relabeling — designed to trigger relabeling path
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, ManyChildrenOfSameParentAllNested) {
    const om_interval root = om_.allocate_root();
    constexpr int k_count = 200;
    std::vector<om_interval> children;
    children.reserve(k_count);
    for (int child_idx = 0; child_idx < k_count; ++child_idx)
        children.push_back(om_.allocate_child_of(root));

    for (int child_idx = 0; child_idx < k_count; ++child_idx) {
        EXPECT_TRUE(is_ancestor(root, children[child_idx]))
            << "child " << child_idx << " not nested in root";
        EXPECT_TRUE(interval_valid(children[child_idx]))
            << "child " << child_idx << " has invalid interval";
    }
}

TEST_F(OrderMaintenanceTest, ManyChildrenOfSameParentAllNonOverlapping) {
    const om_interval root = om_.allocate_root();
    constexpr int k_count = 200;
    std::vector<om_interval> children;
    children.reserve(k_count);
    for (int child_idx = 0; child_idx < k_count; ++child_idx)
        children.push_back(om_.allocate_child_of(root));

    for (int first = 0; first < k_count; ++first) {
        for (int second = first + 1; second < k_count; ++second) {
            EXPECT_TRUE(intervals_non_overlapping(children[first], children[second]))
                << "children " << first << " and " << second << " overlap";
        }
    }
}

TEST_F(OrderMaintenanceTest, ManyChildrenOrderedByAllocation) {
    const om_interval root = om_.allocate_root();
    constexpr int k_count = 200;
    std::vector<om_interval> children;
    children.reserve(k_count);
    for (int child_idx = 0; child_idx < k_count; ++child_idx)
        children.push_back(om_.allocate_child_of(root));

    for (int child_idx = 1; child_idx < k_count; ++child_idx) {
        EXPECT_TRUE(children[child_idx - 1].open < children[child_idx].open)
            << "sibling order violated at index " << child_idx;
    }
}

TEST_F(OrderMaintenanceTest, DeepLinearChainNestingInvariant) {
    constexpr int k_depth = 200;
    std::vector<om_interval> levels;
    levels.reserve(k_depth);
    levels.push_back(om_.allocate_root());
    for (int depth = 1; depth < k_depth; ++depth)
        levels.push_back(om_.allocate_child_of(levels[depth - 1]));

    for (int ancestor_depth = 0; ancestor_depth < k_depth; ++ancestor_depth) {
        for (int descendant_depth = ancestor_depth + 1; descendant_depth < k_depth; ++descendant_depth) {
            EXPECT_TRUE(is_ancestor(levels[ancestor_depth], levels[descendant_depth]))
                << "depth " << ancestor_depth << " not ancestor of depth " << descendant_depth;
        }
    }
}

TEST_F(OrderMaintenanceTest, WideAndDeepMixedInvariantAfterEveryAllocation) {
    // Alternate between adding a sibling to root and adding a child to the
    // last-allocated node.  Verify invariants after every allocation.
    const om_interval root = om_.allocate_root();
    om_interval last_deep  = root;

    constexpr int k_rounds = 50;
    for (int round = 0; round < k_rounds; ++round) {
        const om_interval sibling = om_.allocate_child_of(root);
        EXPECT_TRUE(is_ancestor(root, sibling))
            << "sibling not nested in root at round " << round;
        EXPECT_TRUE(interval_valid(sibling));

        last_deep = om_.allocate_child_of(last_deep);
        EXPECT_TRUE(is_ancestor(root, last_deep))
            << "deep node not nested in root at round " << round;
        EXPECT_TRUE(interval_valid(last_deep));
    }
}

TEST_F(OrderMaintenanceTest, RelabelingPreservesExistingLabelComparisons) {
    // Record the relative order of all labels before a relabeling-triggering
    // burst, then verify every observed a < b relationship still holds.
    const om_interval root = om_.allocate_root();
    constexpr int k_pre = 5;
    std::vector<om_interval> pre_children;
    pre_children.reserve(k_pre);
    for (int pre_idx = 0; pre_idx < k_pre; ++pre_idx)
        pre_children.push_back(om_.allocate_child_of(root));

    // Force relabeling by adding many more siblings.
    constexpr int k_post = 200;
    for (int post_idx = 0; post_idx < k_post; ++post_idx)
        om_.allocate_child_of(root);

    // All pre-relabeling children must still be valid and nested.
    for (int pre_idx = 0; pre_idx < k_pre; ++pre_idx) {
        EXPECT_TRUE(is_ancestor(root, pre_children[pre_idx]))
            << "pre-relabeling child " << pre_idx << " lost nesting";
        EXPECT_TRUE(interval_valid(pre_children[pre_idx]))
            << "pre-relabeling child " << pre_idx << " has invalid interval";
    }

    // Pairwise ordering among pre-relabeling children must also be preserved.
    for (int first = 0; first < k_pre; ++first) {
        for (int second = first + 1; second < k_pre; ++second) {
            EXPECT_TRUE(intervals_non_overlapping(pre_children[first], pre_children[second]))
                << "pre-relabeling children " << first << " and " << second << " now overlap";
        }
    }
}

TEST_F(OrderMaintenanceTest, MultipleRootsAndChildrenAllNested) {
    const om_interval root_a = om_.allocate_root();
    const om_interval root_b = om_.allocate_root();
    const om_interval child_a1 = om_.allocate_child_of(root_a);
    const om_interval child_a2 = om_.allocate_child_of(root_a);
    const om_interval child_b1 = om_.allocate_child_of(root_b);

    EXPECT_TRUE(is_ancestor(root_a, child_a1));
    EXPECT_TRUE(is_ancestor(root_a, child_a2));
    EXPECT_TRUE(is_ancestor(root_b, child_b1));
    EXPECT_FALSE(is_ancestor(root_a, child_b1));
    EXPECT_FALSE(is_ancestor(root_b, child_a1));
}

// ---------------------------------------------------------------------------
// Operator< strict weak order: irreflexivity and asymmetry
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, LabelNotLessThanItself) {
    const om_interval root = om_.allocate_root();
    EXPECT_FALSE(root.open  < root.open);
    EXPECT_FALSE(root.close < root.close);
}

TEST_F(OrderMaintenanceTest, LabelAsymmetry) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    // root.open < child.open → NOT (child.open < root.open)
    EXPECT_TRUE(root.open  < child.open);
    EXPECT_FALSE(child.open < root.open);
}

// ---------------------------------------------------------------------------
// Explicit close-before-next-open ordering for siblings.
// intervals_non_overlapping allows either direction; here we verify the
// specific direction: earlier-allocated sibling's close < later-allocated
// sibling's open.  Catches a bug where open/close pairs are inserted in the
// wrong order relative to each other.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, SiblingCloseBeforeNextSiblingOpen) {
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);
    EXPECT_TRUE(child_a.close < child_b.open);
}

TEST_F(OrderMaintenanceTest, ThreeSiblingsClosesBeforeNextOpens) {
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);
    const om_interval child_c = om_.allocate_child_of(root);
    EXPECT_TRUE(child_a.close < child_b.open);
    EXPECT_TRUE(child_b.close < child_c.open);
}

// ---------------------------------------------------------------------------
// Child's close label is strictly inside parent's close.
// is_ancestor() checks this implicitly, but this explicit test isolates
// the close-side of the nesting invariant.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, ChildCloseStrictlyBeforeParentClose) {
    const om_interval root  = om_.allocate_root();
    const om_interval child = om_.allocate_child_of(root);
    EXPECT_TRUE(child.close < root.close);
}

TEST_F(OrderMaintenanceTest, GrandchildCloseSBeforeParentCloses) {
    const om_interval root       = om_.allocate_root();
    const om_interval child      = om_.allocate_child_of(root);
    const om_interval grandchild = om_.allocate_child_of(child);
    EXPECT_TRUE(grandchild.close < child.close);
    EXPECT_TRUE(child.close      < root.close);
}

// ---------------------------------------------------------------------------
// Relabeling in one subtree must not corrupt cousin labels.
// Catch bug: relabel_segment expands its window past a sibling boundary and
// overwrites that sibling's ranks, breaking previously-valid comparisons.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, RelabelingInOneSubtreeDoesNotCorruptCousin) {
    const om_interval root    = om_.allocate_root();
    const om_interval child_a = om_.allocate_child_of(root);
    const om_interval child_b = om_.allocate_child_of(root);

    // Record cousin's ordering before triggering relabeling.
    const bool cousin_initially_valid   = interval_valid(child_b);
    const bool cousin_initially_nested  = is_ancestor(root, child_b);

    // Pack child_a's subtree with many grandchildren to trigger relabeling.
    constexpr int k_count = 200;
    for (int grand_idx = 0; grand_idx < k_count; ++grand_idx)
        om_.allocate_child_of(child_a);

    // child_b (the cousin) must still be valid and properly nested.
    EXPECT_TRUE(cousin_initially_valid);
    EXPECT_TRUE(cousin_initially_nested);
    EXPECT_TRUE(interval_valid(child_b))   << "cousin interval became invalid after relabeling";
    EXPECT_TRUE(is_ancestor(root, child_b)) << "cousin lost nesting after relabeling";
    // child_a's subtree must not have invaded child_b's interval.
    EXPECT_TRUE(intervals_non_overlapping(child_a, child_b))
        << "child_a and child_b overlap after relabeling";
}

// ---------------------------------------------------------------------------
// After a deep chain forces relabeling, a new sibling added at root must
// still satisfy the full nesting invariant.
// Catch bug: relabeling a deep-chain's crowded area overwrites the parent's
// close rank, making the new sibling appear outside the parent.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, DeepChainThenNewSiblingAtRoot) {
    const om_interval root = om_.allocate_root();
    constexpr int k_depth = 100;
    std::vector<om_interval> chain;
    chain.reserve(k_depth);
    chain.push_back(om_.allocate_child_of(root));
    for (int depth = 1; depth < k_depth; ++depth)
        chain.push_back(om_.allocate_child_of(chain[depth - 1]));

    // Add a new sibling at root level after the deep chain has triggered relabeling.
    const om_interval new_sibling = om_.allocate_child_of(root);
    EXPECT_TRUE(is_ancestor(root, new_sibling))
        << "new root-level sibling not nested in root after deep-chain relabeling";
    EXPECT_TRUE(interval_valid(new_sibling));

    // The new sibling must not overlap with the chain's top node.
    EXPECT_TRUE(intervals_non_overlapping(chain[0], new_sibling))
        << "new sibling overlaps with chain after relabeling";

    // The chain's deepest node must still be properly nested in the root.
    EXPECT_TRUE(is_ancestor(root, chain.back()))
        << "chain tail lost nesting in root after adding new sibling";
}

// ---------------------------------------------------------------------------
// Many roots: all pairwise non-overlapping and all individually valid.
// Catch bug: root allocation uses the wrong insertion point and lands inside
// an existing root's interval.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, ManyRootsAllNonOverlapping) {
    constexpr int k_root_count = 20;
    std::vector<om_interval> roots;
    roots.reserve(k_root_count);
    for (int root_idx = 0; root_idx < k_root_count; ++root_idx)
        roots.push_back(om_.allocate_root());

    for (int root_idx = 0; root_idx < k_root_count; ++root_idx)
        EXPECT_TRUE(interval_valid(roots[root_idx]))
            << "root " << root_idx << " is invalid";

    for (int first = 0; first < k_root_count; ++first) {
        for (int second = first + 1; second < k_root_count; ++second) {
            EXPECT_TRUE(intervals_non_overlapping(roots[first], roots[second]))
                << "roots " << first << " and " << second << " overlap";
        }
    }
}

// ---------------------------------------------------------------------------
// Children of different roots are fully separated from each other.
// Catch bug: allocate_child_of uses the wrong close node and places the child
// inside the wrong root's interval.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, ChildrenOfDifferentRootsNonOverlapping) {
    const om_interval root_a = om_.allocate_root();
    const om_interval root_b = om_.allocate_root();

    std::vector<om_interval> children_a;
    std::vector<om_interval> children_b;
    constexpr int k_per_root = 10;
    children_a.reserve(k_per_root);
    children_b.reserve(k_per_root);
    for (int child_idx = 0; child_idx < k_per_root; ++child_idx) {
        children_a.push_back(om_.allocate_child_of(root_a));
        children_b.push_back(om_.allocate_child_of(root_b));
    }

    for (int a_idx = 0; a_idx < k_per_root; ++a_idx) {
        for (int b_idx = 0; b_idx < k_per_root; ++b_idx) {
            EXPECT_TRUE(intervals_non_overlapping(children_a[a_idx], children_b[b_idx]))
                << "child_a[" << a_idx << "] overlaps with child_b[" << b_idx << "]";
            EXPECT_FALSE(is_ancestor(root_a, children_b[b_idx]))
                << "root_a is ancestor of root_b's child[" << b_idx << "]";
            EXPECT_FALSE(is_ancestor(root_b, children_a[a_idx]))
                << "root_b is ancestor of root_a's child[" << a_idx << "]";
        }
    }
}

// ---------------------------------------------------------------------------
// Stress: relabeling triggered in root_a's subtree must not corrupt root_b's
// labels.  This specifically catches bugs where relabel_segment's window
// expands past a "sibling root" boundary.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, RelabelingDoesNotCorruptSiblingRoot) {
    const om_interval root_a = om_.allocate_root();
    const om_interval root_b = om_.allocate_root();
    const om_interval child_b = om_.allocate_child_of(root_b);

    // Pack root_a with many children to trigger relabeling.
    constexpr int k_count = 200;
    for (int grand_idx = 0; grand_idx < k_count; ++grand_idx)
        om_.allocate_child_of(root_a);

    // root_b and its child must still be valid and properly ordered.
    EXPECT_TRUE(interval_valid(root_b));
    EXPECT_TRUE(interval_valid(child_b));
    EXPECT_TRUE(is_ancestor(root_b, child_b))
        << "root_b's child lost nesting after root_a was packed with children";
    EXPECT_TRUE(intervals_non_overlapping(root_a, root_b))
        << "root_a and root_b overlap after relabeling";
}

// ---------------------------------------------------------------------------
// RNG-based property test: random tree of 200 nodes.
// For every ordered pair (a, b), is_ancestor(a, b) must agree with the
// ground-truth parent table.  Catches any bug that the structural unit tests
// do not exercise because they only test specific small shapes.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, RandomTreeAllPairwiseAncestorRelationships) {
    std::mt19937 rng(42);

    constexpr int k_node_count = 200;
    std::vector<om_interval> intervals;
    std::vector<int> parent_idx;
    intervals.reserve(k_node_count);
    parent_idx.reserve(k_node_count);

    // Always start with one root.
    intervals.push_back(om_.allocate_root());
    parent_idx.push_back(-1);

    for (int idx = 1; idx < k_node_count; ++idx) {
        const bool make_root = (rng() % 6 == 0);
        if (make_root) {
            intervals.push_back(om_.allocate_root());
            parent_idx.push_back(-1);
        } else {
            const int parent = static_cast<int>(rng() % static_cast<uint32_t>(idx));
            intervals.push_back(om_.allocate_child_of(intervals[parent]));
            parent_idx.push_back(parent);
        }
    }

    // Ground-truth: is node a_idx a proper ancestor of b_idx?
    auto known_ancestor = [&](int a_idx, int b_idx) -> bool {
        int cur = parent_idx[b_idx];
        while (cur != -1) {
            if (cur == a_idx) return true;
            cur = parent_idx[cur];
        }
        return false;
    };

    for (int a_idx = 0; a_idx < k_node_count; ++a_idx) {
        for (int b_idx = 0; b_idx < k_node_count; ++b_idx) {
            if (a_idx == b_idx) continue;
            const bool expected = known_ancestor(a_idx, b_idx);
            EXPECT_EQ(is_ancestor(intervals[a_idx], intervals[b_idx]), expected)
                << "is_ancestor(" << a_idx << ", " << b_idx << ") wrong";
        }
    }
}

// Second RNG seed — different tree shape, same thoroughness.
TEST_F(OrderMaintenanceTest, RandomTreeAlternateSeedAllPairwise) {
    std::mt19937 rng(137);

    constexpr int k_node_count = 150;
    std::vector<om_interval> intervals;
    std::vector<int> parent_idx;
    intervals.reserve(k_node_count);
    parent_idx.reserve(k_node_count);

    intervals.push_back(om_.allocate_root());
    parent_idx.push_back(-1);

    for (int idx = 1; idx < k_node_count; ++idx) {
        const bool make_root = (rng() % 8 == 0);
        if (make_root) {
            intervals.push_back(om_.allocate_root());
            parent_idx.push_back(-1);
        } else {
            const int parent = static_cast<int>(rng() % static_cast<uint32_t>(idx));
            intervals.push_back(om_.allocate_child_of(intervals[parent]));
            parent_idx.push_back(parent);
        }
    }

    auto known_ancestor = [&](int a_idx, int b_idx) -> bool {
        int cur = parent_idx[b_idx];
        while (cur != -1) {
            if (cur == a_idx) return true;
            cur = parent_idx[cur];
        }
        return false;
    };

    for (int a_idx = 0; a_idx < k_node_count; ++a_idx) {
        for (int b_idx = 0; b_idx < k_node_count; ++b_idx) {
            if (a_idx == b_idx) continue;
            EXPECT_EQ(is_ancestor(intervals[a_idx], intervals[b_idx]),
                      known_ancestor(a_idx, b_idx))
                << "is_ancestor(" << a_idx << ", " << b_idx << ") wrong (seed 137)";
        }
    }
}

// ---------------------------------------------------------------------------
// Star tree with 1000 children: exercises heavy wide relabeling in one shot.
// Verifies nesting and ordering for all siblings without a nested subtree.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, StarTreeThousandChildrenAllNested) {
    const om_interval root = om_.allocate_root();
    constexpr int k_count = 1000;
    std::vector<om_interval> children;
    children.reserve(k_count);
    for (int child_idx = 0; child_idx < k_count; ++child_idx)
        children.push_back(om_.allocate_child_of(root));

    for (int child_idx = 0; child_idx < k_count; ++child_idx) {
        EXPECT_TRUE(is_ancestor(root, children[child_idx]))
            << "child " << child_idx << " not nested after 1000-child star relabeling";
        EXPECT_TRUE(interval_valid(children[child_idx]));
    }
    for (int child_idx = 1; child_idx < k_count; ++child_idx) {
        EXPECT_TRUE(children[child_idx - 1].close < children[child_idx].open)
            << "sibling order broken at index " << child_idx;
    }
}

// ---------------------------------------------------------------------------
// Balanced binary tree 8 levels deep (255 nodes, 128 leaves).
// Builds the tree level-by-level and verifies all ancestor/non-ancestor
// relationships that a non-binary layout would not stress.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, BalancedBinaryTreeEightLevels) {
    constexpr int k_levels = 8;
    constexpr int k_total  = (1 << k_levels) - 1;  // 255

    // Use deque so we can build entries in place without a default constructor.
    std::vector<om_interval> nodes;
    nodes.reserve(k_total);

    // Allocate all nodes in index order so that parent is always allocated before child.
    nodes.push_back(om_.allocate_root());
    for (int node_idx = 1; node_idx < k_total; ++node_idx) {
        const int parent = (node_idx - 1) / 2;
        nodes.push_back(om_.allocate_child_of(nodes[parent]));
    }

    // Verify: node a is ancestor of node b iff a is on b's path to the root.
    auto is_tree_ancestor = [](int a_idx, int b_idx) -> bool {
        int cur = b_idx;
        while (cur > 0) {
            cur = (cur - 1) / 2;
            if (cur == a_idx) return true;
        }
        return false;
    };

    for (int a_idx = 0; a_idx < k_total; ++a_idx) {
        for (int b_idx = 0; b_idx < k_total; ++b_idx) {
            if (a_idx == b_idx) continue;
            EXPECT_EQ(is_ancestor(nodes[a_idx], nodes[b_idx]),
                      is_tree_ancestor(a_idx, b_idx))
                << "binary tree: is_ancestor(" << a_idx << ", " << b_idx << ") wrong";
        }
    }
}

// ---------------------------------------------------------------------------
// Deep chain of 500 levels: exercises multiple full rounds of relabeling.
// Checks invariants at every 50-level checkpoint, not just at the end,
// so a relabeling that temporarily corrupts the chain would be caught.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, DeepChainFiveHundredLevelsPeriodicCheck) {
    constexpr int k_depth = 500;
    constexpr int k_check_interval = 50;
    std::vector<om_interval> levels;
    levels.reserve(k_depth);
    levels.push_back(om_.allocate_root());

    for (int depth = 1; depth < k_depth; ++depth) {
        levels.push_back(om_.allocate_child_of(levels[depth - 1]));

        if (depth % k_check_interval == 0) {
            // Spot-check: every ancestor in the chain still nests its successor.
            for (int ancestor = 0; ancestor < depth; ++ancestor) {
                EXPECT_TRUE(is_ancestor(levels[ancestor], levels[depth]))
                    << "level " << ancestor << " not ancestor of level " << depth
                    << " (checkpoint at depth " << depth << ")";
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Multiple relabeling rounds: after forcing relabeling several times by
// packing children into a single subtree, verify that new allocations in a
// SIBLING subtree that have never been relabeled still satisfy all invariants.
// Catches a bug where repeated relabeling corrupts the node_by_rank_ptr_ map
// or overwrites sentinel ranks.
// ---------------------------------------------------------------------------

TEST_F(OrderMaintenanceTest, MultipleRelabelingRoundsPreserveSiblingIntegrity) {
    const om_interval root    = om_.allocate_root();
    const om_interval arm_a   = om_.allocate_child_of(root);
    const om_interval arm_b   = om_.allocate_child_of(root);

    // Pack arm_a three times: first 50 grandchildren, then 50 more (2nd round),
    // then 50 more (3rd round).  Each burst likely triggers at least one relabeling.
    for (int grand = 0; grand < 150; ++grand)
        om_.allocate_child_of(arm_a);

    // Now add children to arm_b — it has never been relabeled.
    std::vector<om_interval> b_children;
    b_children.reserve(10);
    for (int child_idx = 0; child_idx < 10; ++child_idx)
        b_children.push_back(om_.allocate_child_of(arm_b));

    for (int child_idx = 0; child_idx < 10; ++child_idx) {
        EXPECT_TRUE(is_ancestor(root,  b_children[child_idx]))
            << "root not ancestor of arm_b child " << child_idx;
        EXPECT_TRUE(is_ancestor(arm_b, b_children[child_idx]))
            << "arm_b not ancestor of its child " << child_idx;
        EXPECT_FALSE(is_ancestor(arm_a, b_children[child_idx]))
            << "arm_a incorrectly became ancestor of arm_b child " << child_idx;
    }
    EXPECT_TRUE(intervals_non_overlapping(arm_a, arm_b))
        << "arm_a and arm_b overlap after multiple relabeling rounds";
}
