#include <gtest/gtest.h>
#include "infrastructure/pud_focus_set.hpp"

namespace {

struct PudFocusSetTest : public ::testing::Test {
    pud_focus_set focus;
    pud_global_node_id axiom{.module = 7, .node = 0};
    pud_global_node_id child{.module = 7, .node = 1};
};

}

TEST_F(PudFocusSetTest, ANewSetContainsNothing) {
    EXPECT_FALSE(focus.contains(axiom));
}

TEST_F(PudFocusSetTest, AnInsertedNodeIsContained) {
    focus.insert(axiom);
    EXPECT_TRUE(focus.contains(axiom));
    EXPECT_FALSE(focus.contains(child));
}

TEST_F(PudFocusSetTest, InsertingTheSameNodeTwiceStillContainsIt) {
    focus.insert(axiom);
    focus.insert(axiom);
    EXPECT_TRUE(focus.contains(axiom));
}
