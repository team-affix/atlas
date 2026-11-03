#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_solution_term_reader.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/rule.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetNodeAxiom {
    MOCK_METHOD(pud_global_node_id, get, (pud_global_node_id));
};

struct MockGetAxiom {
    MOCK_METHOD(const rule&, get_axiom, (pud_global_node_id));
};

struct MockGetNodeBindings {
    MOCK_METHOD(const pud_ubs_state&, get, (pud_global_node_id));
};

struct PudSolutionTermReaderTest : public ::testing::Test {
    MockGetNodeAxiom axioms;
    MockGetAxiom rules;
    MockGetNodeBindings node_bindings;
    pud_solution_term_reader<MockGetNodeAxiom, MockGetAxiom, MockGetNodeBindings> reader{axioms, rules, node_bindings};
    expr head{expr::functor{6, {}}};
    rule axiom{&head, {}, 0};
    pud_ubs_state bindings;
    pud_global_node_id leaf{.module = 9, .node = 4};
    pud_global_node_id axiom_node{.module = 3, .node = 1};
};

}

TEST_F(PudSolutionTermReaderTest, TheTermIsTheAxiomHeadInTheAxiomsModule) {
    EXPECT_CALL(axioms, get(leaf)).WillOnce(Return(axiom_node));
    EXPECT_CALL(rules, get_axiom(axiom_node)).WillOnce(ReturnRef(axiom));
    EXPECT_CALL(node_bindings, get(leaf)).WillOnce(ReturnRef(bindings));
    const pud_solution_term term = reader.read(leaf);
    EXPECT_EQ(term.head, &head);
    EXPECT_EQ(term.module, 3u);
    EXPECT_EQ(term.bindings, bindings);
}
