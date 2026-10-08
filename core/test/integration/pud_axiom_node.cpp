#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/pud_axiom_initializer.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_var_count.hpp"
#include "infrastructure/pud_node_heads.hpp"
#include "infrastructure/pud_node_id_sequencer.hpp"
#include "infrastructure/pud_roots.hpp"
#include "value_objects/rule.hpp"

struct PudAxiomNodeIntegrationTest : public ::testing::Test {
    pud_node_id_sequencer sequencer;
    pud_node_heads heads;
    pud_node_added_body_goals body_goals;
    pud_node_added_var_count var_counts;
    pud_roots roots;
    pud_axiom_initializer<pud_node_id_sequencer, pud_node_heads, pud_node_added_body_goals,
                          pud_node_added_var_count, pud_roots>
        initializer{sequencer, heads, body_goals, var_counts, roots};
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

TEST_F(PudAxiomNodeIntegrationTest, InitializeAxiomRegistersRoot) {
    const pud_node_id id = initializer.initialize_axiom(rule(&head_expr, {}, 0));
    auto co = roots.iterate_roots();
    const auto first = co.next();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first.value(), id);
}

TEST_F(PudAxiomNodeIntegrationTest, TwoAxiomsRegisteredAsRoots) {
    const pud_node_id first_id = initializer.initialize_axiom(rule(&head_expr, {}, 0));
    const pud_node_id second_id = initializer.initialize_axiom(rule(&head_expr, {}, 0));
    auto co = roots.iterate_roots();
    const auto a = co.next();
    const auto b = co.next();
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(a.value(), first_id);
    EXPECT_EQ(b.value(), second_id);
}

TEST_F(PudAxiomNodeIntegrationTest, VarCountStoredExactly) {
    const pud_node_id id = initializer.initialize_axiom(rule(&head_expr, {}, 7));
    EXPECT_EQ(var_counts.get(id), 7u);
}
