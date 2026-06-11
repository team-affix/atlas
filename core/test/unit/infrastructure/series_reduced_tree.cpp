// series_reduced_tree maintains a series-reduced forest (no node with exactly one child).
// insert registers isolated nodes; link wires children to a leaf parent; unlink/erase remove edges or singletons.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <random>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#include "infrastructure/series_reduced_tree.hpp"

using ::testing::UnorderedElementsAre;

namespace {

void collect_forest_nodes(const series_reduced_tree<int>& tree, int node,
        std::unordered_set<int>& nodes) {
    nodes.insert(node);
    try {
        const auto& children = tree.children(node);
        for (int child : children)
            collect_forest_nodes(tree, child, nodes);
    } catch (const std::out_of_range&) {
    }
}

std::unordered_set<int> forest_nodes(const series_reduced_tree<int>& tree) {
    std::unordered_set<int> nodes;
    for (int root : tree.roots())
        collect_forest_nodes(tree, root, nodes);
    return nodes;
}

void expect_series_reduced_invariants(const series_reduced_tree<int>& tree) {
    const auto nodes = forest_nodes(tree);

    for (int root : tree.roots())
        EXPECT_TRUE(nodes.contains(root)) << "root " << root << " not reachable";

    for (int leaf : tree.leaves()) {
        EXPECT_TRUE(nodes.contains(leaf)) << "leaf " << leaf << " not reachable";
        if (!tree.roots().contains(leaf)) {
            const int parent = tree.parent(leaf);
            EXPECT_TRUE(nodes.contains(parent))
                << "leaf " << leaf << " parent " << parent << " not reachable";
        }
    }

    for (int node : nodes) {
        const bool is_root = tree.roots().contains(node);
        const bool is_leaf = tree.leaves().contains(node);

        if (is_root && is_leaf) {
            EXPECT_THROW({ tree.parent(node); }, std::out_of_range);
            EXPECT_THROW({ tree.children(node); }, std::out_of_range);
        } else if (is_leaf && !is_root) {
            EXPECT_THROW({ tree.children(node); }, std::out_of_range);
            const int parent = tree.parent(node);
            EXPECT_TRUE(nodes.contains(parent));
            EXPECT_TRUE(tree.children(parent).contains(node));
        } else if (is_root && !is_leaf) {
            EXPECT_THROW({ tree.parent(node); }, std::out_of_range);
            const auto& children = tree.children(node);
            EXPECT_GE(children.size(), 2u) << "unary branching root " << node;
            for (int child : children) {
                EXPECT_TRUE(nodes.contains(child));
                EXPECT_EQ(tree.parent(child), node);
                EXPECT_FALSE(tree.roots().contains(child));
            }
        } else {
            const int parent = tree.parent(node);
            EXPECT_TRUE(nodes.contains(parent));
            const auto& children = tree.children(node);
            EXPECT_GE(children.size(), 2u) << "unary branching node " << node;
            EXPECT_TRUE(tree.children(parent).contains(node));
            for (int child : children) {
                EXPECT_TRUE(nodes.contains(child));
                EXPECT_EQ(tree.parent(child), node);
            }
        }
    }
}

bool try_random_link(series_reduced_tree<int>& tree, std::mt19937& rng) {
    std::vector<int> leaf_parents;
    std::vector<int> root_children;
    for (int id = 1; id <= 15; ++id) {
        if (tree.leaves().contains(id))
            leaf_parents.push_back(id);
        if (tree.roots().contains(id))
            root_children.push_back(id);
    }
    if (leaf_parents.empty() || root_children.empty())
        return false;

    std::uniform_int_distribution<size_t> parent_dist(0, leaf_parents.size() - 1);
    const int parent = leaf_parents[parent_dist(rng)];

    std::unordered_set<int> ancestors;
    for (int cur = parent; ; ) {
        try {
            cur = tree.parent(cur);
            ancestors.insert(cur);
        } catch (const std::out_of_range&) {
            break;
        }
    }

    std::vector<int> candidates;
    for (int child : root_children) {
        if (child != parent && !ancestors.contains(child))
            candidates.push_back(child);
    }
    if (candidates.empty())
        return false;

    std::shuffle(candidates.begin(), candidates.end(), rng);
    std::uniform_int_distribution<size_t> count_dist(1, candidates.size());
    const size_t count = count_dist(rng);

    std::set<int> children(candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>(count));
    return tree.link(parent, children);
}

} // namespace

struct SeriesReducedTreeTest : public ::testing::Test {
    series_reduced_tree<int> tree;
};

TEST_F(SeriesReducedTreeTest, InsertAddsIsolatedNode) {
    ASSERT_TRUE(tree.insert(1));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(1));
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    ASSERT_TRUE(tree.erase(1));
    EXPECT_TRUE(tree.roots().empty());
    EXPECT_TRUE(tree.leaves().empty());
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, InsertDuplicateReturnsFalse) {
    ASSERT_TRUE(tree.insert(1));
    EXPECT_FALSE(tree.insert(1));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(1));
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, LinkMultipleChildren) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20));
    EXPECT_EQ(tree.parent(10), 1);
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, LinkSingletonCollapsesParent) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.link(1, {10}));
    EXPECT_THROW({ tree.children(1); }, std::out_of_range);
    EXPECT_THROW({ tree.children(10); }, std::out_of_range);
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(10));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10));
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    EXPECT_FALSE(tree.erase(1));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(10));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10));
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, LinkChainCollapses) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.insert(100));
    ASSERT_TRUE(tree.insert(1000));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.link(10, {100}));
    ASSERT_TRUE(tree.link(100, {1000}));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(1000, 20));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(20, 1000));
    EXPECT_EQ(tree.parent(1000), 1);
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(100); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    EXPECT_THROW({ tree.children(10); }, std::out_of_range);
    EXPECT_THROW({ tree.children(100); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, MultipleRoots) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.insert(30));
    ASSERT_TRUE(tree.insert(40));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.link(2, {30, 40}));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
    EXPECT_THAT(tree.children(2), UnorderedElementsAre(30, 40));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 2));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20, 30, 40));
    EXPECT_EQ(tree.parent(10), 1);
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_EQ(tree.parent(30), 2);
    EXPECT_EQ(tree.parent(40), 2);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(2); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, LinkFailsParentNotLeaf) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.insert(30));
    EXPECT_FALSE(tree.link(1, {30}));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 30));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20, 30));
    EXPECT_EQ(tree.parent(10), 1);
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_THROW({ tree.parent(30); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, LinkFailsChildNotRoot) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.insert(2));
    EXPECT_FALSE(tree.link(2, {10}));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 2));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20, 2));
    EXPECT_EQ(tree.parent(10), 1);
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_THROW({ tree.parent(2); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, LinkFailsUnknownParent) {
    EXPECT_FALSE(tree.link(99, {1}));
    ASSERT_TRUE(tree.insert(1));
    EXPECT_FALSE(tree.link(99, {1}));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(1));
}

TEST_F(SeriesReducedTreeTest, InsertFailsWhenAlreadyInForest) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    EXPECT_FALSE(tree.insert(10));
    EXPECT_FALSE(tree.insert(1));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20));
    EXPECT_EQ(tree.parent(10), 1);
    EXPECT_EQ(tree.parent(20), 1);
}

TEST_F(SeriesReducedTreeTest, LinkEmptyChildrenRemovesIsolatedParent) {
    ASSERT_TRUE(tree.insert(1));
    EXPECT_TRUE(tree.link(1, {}));
    EXPECT_TRUE(tree.roots().empty());
    EXPECT_TRUE(tree.leaves().empty());
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    EXPECT_THROW({ tree.children(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, LinkEmptyChildrenRemovesNonRootLeaf) {
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.link(0, {1, 10}));
    EXPECT_TRUE(tree.link(1, {}));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(10));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10));
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    EXPECT_THROW({ tree.children(0); }, std::out_of_range);
    EXPECT_THROW({ tree.children(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, LinkEmptyChildrenLeavesBranchingGrandparentIntact) {
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.insert(3));
    ASSERT_TRUE(tree.link(0, {1, 2, 3}));
    EXPECT_TRUE(tree.link(1, {}));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(0));
    EXPECT_THAT(tree.children(0), UnorderedElementsAre(2, 3));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(2, 3));
    EXPECT_EQ(tree.parent(2), 0);
    EXPECT_EQ(tree.parent(3), 0);
    expect_series_reduced_invariants(tree);
}

TEST_F(SeriesReducedTreeTest, LinkEmptyChildrenNullaryUnaryCascadeThroughGrandparent) {
    ASSERT_TRUE(tree.insert(5));
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(5, {0, 1}));
    ASSERT_TRUE(tree.link(0, {10, 20}));
    EXPECT_TRUE(tree.link(10, {}));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(5));
    EXPECT_THAT(tree.children(5), UnorderedElementsAre(1, 20));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(1, 20));
    EXPECT_EQ(tree.parent(20), 5);
    EXPECT_THROW({ tree.parent(0); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
    EXPECT_THROW({ tree.children(0); }, std::out_of_range);
    expect_series_reduced_invariants(tree);
}

TEST_F(SeriesReducedTreeTest, LinkEmptyChildrenSequentialNullaryCascadeEmptiesForest) {
    // Each empty-link nullary-removes a leaf and may recursively try_reduce
    // ancestors (unary collapse when one sibling remains).
    ASSERT_TRUE(tree.insert(5));
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.link(5, {0, 1, 2}));
    ASSERT_TRUE(tree.link(1, {}));
    EXPECT_THAT(tree.children(5), UnorderedElementsAre(0, 2));
    ASSERT_TRUE(tree.link(2, {}));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(0));
    EXPECT_TRUE(tree.link(0, {}));
    EXPECT_TRUE(tree.roots().empty());
    EXPECT_TRUE(tree.leaves().empty());
    expect_series_reduced_invariants(tree);
}

TEST_F(SeriesReducedTreeTest, UnlinkRestoresChildAsRootWhenParentStillBranches) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.insert(30));
    ASSERT_TRUE(tree.link(1, {10, 20, 30}));
    ASSERT_TRUE(tree.unlink(10));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 10));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20, 30));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(20, 30));
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_EQ(tree.parent(30), 1);
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
    ASSERT_TRUE(tree.erase(10));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(20, 30));
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_EQ(tree.parent(30), 1);
}

TEST_F(SeriesReducedTreeTest, UnlinkUnaryCollapsesParent) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.unlink(10));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(10, 20));
    EXPECT_THROW({ tree.children(1); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(20); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    EXPECT_FALSE(tree.erase(1));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(10, 20));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20));
}

TEST_F(SeriesReducedTreeTest, UnlinkAbsorbsFormerBranchingParent) {
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.link(0, {1, 2}));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.unlink(2));
    EXPECT_FALSE(tree.erase(0));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 2));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(2, 10, 20));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
    EXPECT_EQ(tree.parent(10), 1);
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(2); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(0); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, UnlinkTwiceReturnsFalse) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.unlink(10));
    EXPECT_FALSE(tree.unlink(10));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(10, 20));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20));
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(20); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, LinkUnaryInternalCollapsesUnderGrandparent) {
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.link(0, {1, 2}));
    ASSERT_TRUE(tree.link(1, {10}));
    EXPECT_THAT(tree.children(0), UnorderedElementsAre(10, 2));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(0));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(2, 10));
    EXPECT_EQ(tree.parent(10), 0);
    EXPECT_EQ(tree.parent(2), 0);
    EXPECT_THROW({ tree.children(1); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, UnlinkAllChildrenSequentially) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.insert(30));
    ASSERT_TRUE(tree.link(1, {10, 20, 30}));
    ASSERT_TRUE(tree.unlink(10));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(20, 30));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 10));
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_EQ(tree.parent(30), 1);
    ASSERT_TRUE(tree.unlink(20));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(10, 20, 30));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20, 30));
    EXPECT_THROW({ tree.children(1); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(20); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(30); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, UnlinkFailsNoEdge) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    EXPECT_FALSE(tree.unlink(99));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20));
    EXPECT_EQ(tree.parent(10), 1);
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_THROW({ tree.parent(99); }, std::out_of_range);
    ASSERT_TRUE(tree.insert(30));
    EXPECT_FALSE(tree.unlink(30));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 30));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20, 30));
    EXPECT_THROW({ tree.parent(30); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, EraseIsolatedSucceeds) {
    ASSERT_TRUE(tree.insert(1));
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    ASSERT_TRUE(tree.erase(1));
    EXPECT_TRUE(tree.roots().empty());
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, EraseLinkedLeafFailsUntilUnlink) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    EXPECT_FALSE(tree.erase(10));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20));
    EXPECT_EQ(tree.parent(10), 1);
    EXPECT_EQ(tree.parent(20), 1);
    ASSERT_TRUE(tree.unlink(10));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(10, 20));
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(20); }, std::out_of_range);
    ASSERT_TRUE(tree.erase(10));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(20));
    EXPECT_THROW({ tree.parent(20); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, EraseLinkedFails) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    EXPECT_FALSE(tree.erase(10));
    EXPECT_FALSE(tree.erase(1));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(10, 20));
    EXPECT_EQ(tree.parent(10), 1);
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, EraseMissingFails) {
    EXPECT_FALSE(tree.erase(99));
    EXPECT_TRUE(tree.roots().empty());
    EXPECT_TRUE(tree.leaves().empty());
    EXPECT_THROW({ tree.parent(99); }, std::out_of_range);
    EXPECT_THROW({ tree.children(99); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, UnlinkThenTryReduceOnParent) {
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.link(0, {1, 2}));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    ASSERT_TRUE(tree.unlink(20));
    EXPECT_THAT(tree.children(0), UnorderedElementsAre(10, 2));
    EXPECT_EQ(tree.parent(10), 0);
    EXPECT_EQ(tree.parent(2), 0);
    EXPECT_THROW({ tree.parent(20); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    EXPECT_THROW({ tree.children(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, ChildrenMissingParentThrows) {
    EXPECT_THROW({ tree.children(99); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, ParentMissingChildThrows) {
    EXPECT_THROW({ tree.parent(99); }, std::out_of_range);
    ASSERT_TRUE(tree.insert(1));
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, ClearEmptiesForest) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.link(1, {10}));
    tree.clear();
    EXPECT_TRUE(tree.roots().empty());
    EXPECT_TRUE(tree.leaves().empty());
    EXPECT_THROW({ tree.children(1); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, ReinsertAbsorbedIdSucceeds) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.link(1, {10}));
    ASSERT_TRUE(tree.insert(1));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1, 10));
    EXPECT_THROW({ tree.parent(10); }, std::out_of_range);
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, ReinsertAfterEraseSucceeds) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.erase(1));
    ASSERT_TRUE(tree.insert(1));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(1));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(1));
    EXPECT_THROW({ tree.parent(1); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, ClearThenReuse) {
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.link(1, {10}));
    tree.clear();
    ASSERT_TRUE(tree.insert(5));
    ASSERT_TRUE(tree.insert(50));
    ASSERT_TRUE(tree.insert(51));
    ASSERT_TRUE(tree.link(5, {50, 51}));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(5));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(50, 51));
    EXPECT_EQ(tree.parent(50), 5);
    EXPECT_EQ(tree.parent(51), 5);
}

TEST_F(SeriesReducedTreeTest, NonRootBranchingParent) {
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.insert(20));
    ASSERT_TRUE(tree.link(0, {1, 2}));
    ASSERT_TRUE(tree.link(1, {10, 20}));
    EXPECT_THAT(tree.children(0), UnorderedElementsAre(1, 2));
    EXPECT_THAT(tree.children(1), UnorderedElementsAre(10, 20));
    EXPECT_THAT(tree.roots(), UnorderedElementsAre(0));
    EXPECT_THAT(tree.leaves(), UnorderedElementsAre(2, 10, 20));
    EXPECT_EQ(tree.parent(1), 0);
    EXPECT_EQ(tree.parent(2), 0);
    EXPECT_EQ(tree.parent(10), 1);
    EXPECT_EQ(tree.parent(20), 1);
    EXPECT_THROW({ tree.parent(0); }, std::out_of_range);
}

TEST_F(SeriesReducedTreeTest, InvariantsHoldOnEmptyForest) {
    expect_series_reduced_invariants(tree);
}

TEST_F(SeriesReducedTreeTest, InvariantsHoldAfterRepresentativeSequence) {
    ASSERT_TRUE(tree.insert(0));
    ASSERT_TRUE(tree.insert(1));
    ASSERT_TRUE(tree.insert(2));
    ASSERT_TRUE(tree.insert(10));
    ASSERT_TRUE(tree.link(0, {1, 2}));
    expect_series_reduced_invariants(tree);

    ASSERT_TRUE(tree.link(1, {10}));
    expect_series_reduced_invariants(tree);

    ASSERT_TRUE(tree.unlink(2));
    expect_series_reduced_invariants(tree);

    ASSERT_TRUE(tree.insert(99));
    ASSERT_TRUE(tree.link(99, {2}));
    expect_series_reduced_invariants(tree);
}

TEST_F(SeriesReducedTreeTest, FuzzRandomOperations) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> op_dist(0, 3);
    std::uniform_int_distribution<int> id_dist(1, 15);

    expect_series_reduced_invariants(tree);

    for (int step = 0; step < 1000; ++step) {
        switch (op_dist(rng)) {
        case 0:
            tree.insert(id_dist(rng));
            break;
        case 1:
            tree.erase(id_dist(rng));
            break;
        case 2:
            try_random_link(tree, rng);
            break;
        case 3:
            tree.unlink(id_dist(rng));
            break;
        }
        expect_series_reduced_invariants(tree);
    }
}
