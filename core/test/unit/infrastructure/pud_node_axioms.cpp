#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_node_axioms.hpp"
#include "pud_test_node.hpp"

namespace {

struct PudNodeAxiomsTest : public ::testing::Test {
    pud_node_axioms axioms;
    const pud_global_node_id axiom = pud_test_node(2);
    const pud_global_node_id child = pud_test_node(5);
};

}

TEST_F(PudNodeAxiomsTest, AStoredAxiomReadsBackForItsNode) {
    axioms.store(child, axiom);
    EXPECT_EQ(axioms.get(child), axiom);
}

TEST_F(PudNodeAxiomsTest, AnAxiomStoredAsItsOwnRootReadsBackAsItself) {
    axioms.store(axiom, axiom);
    EXPECT_EQ(axioms.get(axiom), axiom);
}

TEST_F(PudNodeAxiomsTest, StoringTheSameNodeTwiceFails) {
    axioms.store(child, axiom);
    EXPECT_THROW(axioms.store(child, axiom), std::logic_error);
}
