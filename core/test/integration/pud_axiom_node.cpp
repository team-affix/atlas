#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_axiom_initializer.hpp"
#include "infrastructure/pud_leaf_nodes.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_var_count.hpp"
#include "infrastructure/pud_node_heads.hpp"
#include "infrastructure/pud_node_id_sequencer.hpp"
#include "infrastructure/pud_root_nodes.hpp"
#include "value_objects/rule.hpp"

struct PudAxiomNodeIntegrationTest : public ::testing::Test {
    pud_node_id_sequencer sequencer;
    pud_node_heads heads;
    pud_node_added_body_goals body_goals;
    pud_node_added_var_count var_counts;
    pud_root_nodes roots;
    pud_leaf_nodes leaves;
    pud_axiom_initializer<pud_node_id_sequencer, pud_node_heads, pud_node_added_body_goals,
                          pud_node_added_var_count, pud_root_nodes, pud_leaf_nodes>
        initializer{sequencer, heads, body_goals, var_counts, roots, leaves};
    expr head_expr{expr::var{0}};
    expr body_a{expr::var{1}};
    expr body_b{expr::var{2}};
};

TEST_F(PudAxiomNodeIntegrationTest, InitializeAxiomStoresHeadDirectly) {
    const pud_node_id id = initializer.initialize_axiom(rule(&head_expr, {}, 4));
    EXPECT_EQ(heads.get(id), &head_expr);
    EXPECT_EQ(var_counts.get(id), 4u);
    EXPECT_TRUE(body_goals.get(id).empty());
}

TEST_F(PudAxiomNodeIntegrationTest, PayloadStillReadsBackAfterASecondAxiom) {
    const pud_node_id first = initializer.initialize_axiom(rule(&head_expr, {}, 1));
    initializer.initialize_axiom(rule(&head_expr, {}, 2));
    EXPECT_EQ(heads.get(first), &head_expr);
    EXPECT_EQ(var_counts.get(first), 1u);
}

TEST_F(PudAxiomNodeIntegrationTest, HeadAndTwoBodyGoalsLandInOrder) {
    const pud_node_id id = initializer.initialize_axiom(rule(&head_expr, {&body_a, &body_b}, 0));
    EXPECT_EQ(heads.get(id), &head_expr);
    const auto& goals = body_goals.get(id);
    ASSERT_EQ(goals.size(), 2u);
    EXPECT_EQ(goals[0], &body_a);
    EXPECT_EQ(goals[1], &body_b);
}

TEST_F(PudAxiomNodeIntegrationTest, InitializeAxiomMarksARoot) {
    const pud_node_id id = initializer.initialize_axiom(rule(&head_expr, {}, 0));
    EXPECT_TRUE(roots.check_root(id));
    EXPECT_EQ(roots.get(), (std::vector<pud_node_id>{id}));
}

TEST_F(PudAxiomNodeIntegrationTest, InitializeAxiomMarksALeaf) {
    const pud_node_id id = initializer.initialize_axiom(rule(&head_expr, {&body_a}, 0));
    EXPECT_TRUE(leaves.check_leaf(id));
}

TEST_F(PudAxiomNodeIntegrationTest, TwoAxiomsAreRootsInOrder) {
    const pud_node_id first_id = initializer.initialize_axiom(rule(&head_expr, {}, 0));
    const pud_node_id second_id = initializer.initialize_axiom(rule(&head_expr, {}, 0));
    EXPECT_EQ(roots.get(), (std::vector<pud_node_id>{first_id, second_id}));
}

TEST_F(PudAxiomNodeIntegrationTest, VarCountStoredExactly) {
    const pud_node_id id = initializer.initialize_axiom(rule(&head_expr, {}, 7));
    EXPECT_EQ(var_counts.get(id), 7u);
}
