#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_axiom_initializer.hpp"
#include "infrastructure/pud_axioms.hpp"
#include "infrastructure/pud_leaf_nodes.hpp"
#include "infrastructure/pud_node_added_body_goals.hpp"
#include "infrastructure/pud_node_added_var_count.hpp"
#include "infrastructure/pud_node_heads.hpp"
#include "infrastructure/pud_node_id_sequencer.hpp"
#include "infrastructure/pud_root_nodes.hpp"
#include "infrastructure/pud_conjecture_initializer.hpp"
#include "value_objects/rule.hpp"

namespace {

struct MockDescent {
    int tag;
};

struct MockMakeRootDescent {
    MOCK_METHOD(MockDescent, make, (pud_node_id));
};

struct MockActivateLeaf {
    MOCK_METHOD(void, activate, (pud_node_id, MockDescent));
};

}  // namespace

struct PudConjectureNodeIntegrationTest : public ::testing::Test {
    pud_node_id_sequencer sequencer;
    pud_node_heads heads;
    pud_node_added_body_goals body_goals;
    pud_node_added_var_count var_counts;
    pud_root_nodes roots;
    pud_leaf_nodes leaves;
    pud_axioms axioms;
    pud_axiom_initializer<pud_node_id_sequencer, pud_node_heads, pud_node_added_body_goals,
                          pud_node_added_var_count, pud_root_nodes, pud_leaf_nodes, pud_axioms>
        axiom_initializer{sequencer, heads, body_goals, var_counts, roots, leaves, axioms};
    ::testing::NiceMock<MockMakeRootDescent> make_root_descent;
    ::testing::NiceMock<MockActivateLeaf> activate_leaf;
    pud_conjecture_initializer<pud_node_id_sequencer, pud_node_added_body_goals,
                               pud_node_added_var_count, pud_root_nodes, pud_leaf_nodes,
                               ::testing::NiceMock<MockMakeRootDescent>, ::testing::NiceMock<MockActivateLeaf>>
        conjecture_initializer{sequencer, body_goals, var_counts, roots, leaves, make_root_descent, activate_leaf};
    expr head_expr{expr::var{0}};
    expr goal_a{expr::var{1}};
    expr goal_b{expr::var{2}};
};

TEST_F(PudConjectureNodeIntegrationTest, ConjectureIsAHeadlessRootLeaf) {
    const pud_node_id query = conjecture_initializer.initialize_conjecture({&goal_a, &goal_b}, 3);
    EXPECT_TRUE(roots.check_root(query));
    EXPECT_TRUE(leaves.check_leaf(query));
    EXPECT_EQ(body_goals.get(query), (std::vector<const expr*>{&goal_a, &goal_b}));
    EXPECT_EQ(var_counts.get(query), 3u);
    EXPECT_THROW(heads.get(query), std::out_of_range);
}

TEST_F(PudConjectureNodeIntegrationTest, ConjecturesAreNeverListedAmongAxioms) {
    const pud_node_id first = axiom_initializer.initialize_axiom(rule(&head_expr, {}, 0));
    const pud_node_id query = conjecture_initializer.initialize_conjecture({&goal_a}, 1);
    const pud_node_id second = axiom_initializer.initialize_axiom(rule(&head_expr, {&goal_b}, 1));
    EXPECT_EQ(axioms.get(), (std::vector<pud_node_id>{first, second}));
    EXPECT_NE(query, first);
    EXPECT_NE(query, second);
}
