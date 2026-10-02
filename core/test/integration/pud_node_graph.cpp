#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_call_sites.hpp"
#include "infrastructure/pud_children.hpp"
#include "infrastructure/pud_leaves.hpp"
#include "infrastructure/pud_node_pool.hpp"
#include "infrastructure/pud_parents.hpp"
#include "infrastructure/pud_refuted_nodes.hpp"

struct PudNodeGraphIntegrationTest : public ::testing::Test {
    pud_node_pool pool;
    pud_parents parents;
    pud_children children;
    pud_leaves leaves;
    pud_call_sites call_sites;
    pud_refuted_nodes refuted;

    const pud_node* make() {
        return pool.make({}, {}, 0);
    }
};

TEST_F(PudNodeGraphIntegrationTest, StoredFactsStillReadBackAfterMoreNodesAreMade) {
    const pud_node* node = make();
    const pud_node* parent = make();
    const pud_node* child = make();
    std::vector<const pud_node*> stored_children{child};
    parents.store(node, parent);
    children.store(node, stored_children);
    leaves.set_leaf(node);
    call_sites.store(node, 4);
    refuted.set_refuted(node);

    make();
    make();

    EXPECT_EQ(parents.get(node), parent);
    ASSERT_EQ(children.get(node).size(), 1u);
    EXPECT_EQ(children.get(node)[0], child);
    EXPECT_TRUE(leaves.check_leaf(node));
    EXPECT_EQ(call_sites.get(node), 4u);
    EXPECT_TRUE(refuted.check_refuted(node));
}

TEST_F(PudNodeGraphIntegrationTest, StoredEmptyChildListReadsBackEmpty) {
    const pud_node* node = make();
    children.store(node, {});
    make();
    EXPECT_TRUE(children.get(node).empty());
}

TEST_F(PudNodeGraphIntegrationTest, UnsetLeafLeavesParentCallSiteAndRefutedMark) {
    const pud_node* node = make();
    const pud_node* parent = make();
    parents.store(node, parent);
    call_sites.store(node, 2);
    leaves.set_leaf(node);
    refuted.set_refuted(node);

    leaves.unset_leaf(node);

    EXPECT_FALSE(leaves.check_leaf(node));
    EXPECT_EQ(parents.get(node), parent);
    EXPECT_EQ(call_sites.get(node), 2u);
    EXPECT_TRUE(refuted.check_refuted(node));
}

TEST_F(PudNodeGraphIntegrationTest, OneNodeIsALeafWithParentCallSiteAndTwoChildren) {
    const pud_node* node = make();
    const pud_node* parent = make();
    const pud_node* left = make();
    const pud_node* right = make();
    parents.store(node, parent);
    children.store(node, {left, right});
    leaves.set_leaf(node);
    call_sites.store(node, 7);

    EXPECT_TRUE(leaves.check_leaf(node));
    EXPECT_EQ(parents.get(node), parent);
    EXPECT_EQ(call_sites.get(node), 7u);
    ASSERT_EQ(children.get(node).size(), 2u);
    EXPECT_EQ(children.get(node)[0], left);
    EXPECT_EQ(children.get(node)[1], right);
    EXPECT_FALSE(refuted.check_refuted(node));
}
