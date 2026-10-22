#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_call_sites.hpp"
#include "infrastructure/pud_children.hpp"
#include "infrastructure/pud_leaf_nodes.hpp"
#include "infrastructure/pud_node_id_sequencer.hpp"

struct PudNodeGraphIntegrationTest : public ::testing::Test {
    pud_node_id_sequencer seq;
    pud_children children;
    pud_leaf_nodes leaves;
    pud_call_sites call_sites;

    pud_node_id make() { return seq.next(); }
};

TEST_F(PudNodeGraphIntegrationTest, StoredFactsStillReadBackAfterMoreNodesAreMade) {
    const pud_node_id node  = make();
    const pud_node_id child = make();
    children.store(node, {child});
    leaves.set_leaf(node);
    call_sites.store(node, 4);

    make();
    make();

    ASSERT_EQ(children.get(node).size(), 1u);
    EXPECT_EQ(children.get(node)[0], child);
    EXPECT_TRUE(leaves.check_leaf(node));
    EXPECT_EQ(call_sites.get(node), 4u);
}

TEST_F(PudNodeGraphIntegrationTest, StoredEmptyChildListReadsBackEmpty) {
    const pud_node_id node = make();
    children.store(node, {});
    make();
    EXPECT_TRUE(children.get(node).empty());
}

TEST_F(PudNodeGraphIntegrationTest, UnsetLeafLeavesCallSite) {
    const pud_node_id node = make();
    call_sites.store(node, 2);
    leaves.set_leaf(node);

    leaves.unset_leaf(node);

    EXPECT_FALSE(leaves.check_leaf(node));
    EXPECT_EQ(call_sites.get(node), 2u);
}

TEST_F(PudNodeGraphIntegrationTest, OneNodeIsALeafWithCallSiteAndTwoChildren) {
    const pud_node_id node  = make();
    const pud_node_id left  = make();
    const pud_node_id right = make();
    children.store(node, {left, right});
    leaves.set_leaf(node);
    call_sites.store(node, 7);

    EXPECT_TRUE(leaves.check_leaf(node));
    EXPECT_EQ(call_sites.get(node), 7u);
    ASSERT_EQ(children.get(node).size(), 2u);
    EXPECT_EQ(children.get(node)[0], left);
    EXPECT_EQ(children.get(node)[1], right);
}
