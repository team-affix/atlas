#include <gtest/gtest.h>
#include "infrastructure/pud_node_states.hpp"

namespace {

struct PudNodeStatesTest : public ::testing::Test {
    pud_node_states states;
};

}

TEST_F(PudNodeStatesTest, ANewNodeIsALeaf) {
    states.mark(0, pud_node_state::leaf);
    EXPECT_EQ(states.get(0), pud_node_state::leaf);
}

TEST_F(PudNodeStatesTest, ALeafCanBeExpandedOrRefuted) {
    states.mark(0, pud_node_state::leaf);
    states.mark(1, pud_node_state::leaf);
    states.mark(0, pud_node_state::expanded);
    states.mark(1, pud_node_state::refuted);
    EXPECT_EQ(states.get(0), pud_node_state::expanded);
    EXPECT_EQ(states.get(1), pud_node_state::refuted);
}
