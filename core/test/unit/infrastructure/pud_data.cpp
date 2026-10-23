#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_data.hpp"
#include "value_objects/expr.hpp"

using ::testing::ElementsAre;
using ::testing::IsEmpty;

namespace {

struct PudDataTest : public ::testing::Test {
    pud_data data;
    expr head{expr::functor{1, {}}};
    expr goal{expr::functor{2, {}}};
    expr value{expr::functor{3, {}}};
    rule axiom{&head, {&goal}, 2};

    pud_node_delta inference_of(pud_node_id parent, pud_body_goal_id call_site) {
        return pud_node_delta{
            .parent = parent,
            .call_site = call_site,
            .substitutions = {pud_substitution{.var_idx = 0, .value = &value}},
            .body_goals = {&goal},
            .var_count = 1,
        };
    }
};

}

TEST_F(PudDataTest, NodesAreNumberedInTheOrderTheyAreStored) {
    EXPECT_EQ(data.store_axiom(axiom), 0u);
    EXPECT_EQ(data.store_axiom(axiom), 1u);
    EXPECT_EQ(data.store_inference(inference_of(0, 0)), 2u);
}

TEST_F(PudDataTest, AnAxiomIsStoredAsItsRuleWithoutAParent) {
    const pud_node_id root = data.store_axiom(axiom);
    EXPECT_EQ(data.get_axiom(root), axiom);
    EXPECT_EQ(data.get_parent(root), std::nullopt);
}

TEST_F(PudDataTest, AnInferenceIsStoredAsItsDeltaUnderItsParent) {
    const pud_node_id root = data.store_axiom(axiom);
    const pud_node_id child = data.store_inference(inference_of(root, 0));
    EXPECT_EQ(data.get_inference(child), inference_of(root, 0));
    EXPECT_EQ(data.get_parent(child), std::optional<pud_node_id>{root});
}

TEST_F(PudDataTest, ANodeIsALeafUntilItsFirstChildIsStored) {
    const pud_node_id root = data.store_axiom(axiom);
    EXPECT_TRUE(data.check_leaf(root));
    EXPECT_THAT(data.get_children(root), IsEmpty());
    const pud_node_id child = data.store_inference(inference_of(root, 0));
    EXPECT_FALSE(data.check_leaf(root));
    EXPECT_TRUE(data.check_leaf(child));
}

TEST_F(PudDataTest, ChildrenAreKeptInTheOrderTheyAreStored) {
    const pud_node_id root = data.store_axiom(axiom);
    const pud_node_id first = data.store_inference(inference_of(root, 0));
    const pud_node_id second = data.store_inference(inference_of(root, 0));
    const pud_node_id grandchild = data.store_inference(inference_of(first, 0));
    EXPECT_THAT(data.get_children(root), ElementsAre(first, second));
    EXPECT_THAT(data.get_children(first), ElementsAre(grandchild));
}

TEST_F(PudDataTest, StoredNodesStayInPlaceWhileLaterNodesAreStored) {
    const pud_node_id root = data.store_axiom(axiom);
    const pud_node_id child = data.store_inference(inference_of(root, 0));
    const rule& stored_axiom = data.get_axiom(root);
    const pud_node_delta& stored_inference = data.get_inference(child);
    const std::vector<pud_node_id>& children = data.get_children(root);
    for (int stored = 0; stored < 1000; ++stored)
        data.store_axiom(axiom);
    EXPECT_EQ(&stored_axiom, &data.get_axiom(root));
    EXPECT_EQ(&stored_inference, &data.get_inference(child));
    EXPECT_EQ(&children, &data.get_children(root));
}
