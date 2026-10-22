#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_axioms.hpp"

struct PudAxiomsTest : public ::testing::Test {
    pud_axioms axioms;
    pud_node_id node = 1;
    pud_node_id other = 2;
};

TEST_F(PudAxiomsTest, GetIsEmptyBeforeAnyAxiomIsMarked) {
    EXPECT_TRUE(axioms.get().empty());
}

TEST_F(PudAxiomsTest, GetListsAxiomsInMarkingOrder) {
    axioms.mark_axiom(other);
    axioms.mark_axiom(node);
    EXPECT_EQ(axioms.get(), (std::vector<pud_node_id>{other, node}));
}

TEST_F(PudAxiomsTest, SecondMarkAxiomThrows) {
    axioms.mark_axiom(node);
    EXPECT_THROW(axioms.mark_axiom(node), std::logic_error);
}

TEST_F(PudAxiomsTest, CheckAxiomIsFalseWhenNeverMarked) {
    EXPECT_FALSE(axioms.check_axiom(node));
}

TEST_F(PudAxiomsTest, MarkedAxiomIsAnAxiomAndNoOtherIs) {
    axioms.mark_axiom(node);
    EXPECT_TRUE(axioms.check_axiom(node));
    EXPECT_FALSE(axioms.check_axiom(other));
}
