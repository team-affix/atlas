#include <algorithm>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_child_shuffler.hpp"
#include "pud_test_node.hpp"

namespace {

struct PudChildShufflerTest : public ::testing::Test {
    pud_child_shuffler shuffler;

    std::vector<pud_inference> children(size_t count) {
        std::vector<pud_inference> made;
        for (size_t index = 0; index < count; ++index)
            made.push_back(pud_inference{
                .parent = 4, .call_site = 1, .substitutions = {}, .body_goals = {},
                .var_count = static_cast<uint32_t>(index)});
        return made;
    }

    std::vector<uint32_t> order_of(const std::vector<pud_inference>& shuffled) {
        std::vector<uint32_t> order;
        for (const pud_inference& child : shuffled)
            order.push_back(child.var_count);
        return order;
    }
};

}

TEST_F(PudChildShufflerTest, NoChildOrOneChildIsLeftAsItIs) {
    EXPECT_TRUE(shuffler.shuffle(pud_test_node(4), children(0)).empty());
    EXPECT_EQ(shuffler.shuffle(pud_test_node(4), children(1)), children(1));
}

TEST_F(PudChildShufflerTest, TheShuffleKeepsEveryChildExactlyOnce) {
    std::vector<uint32_t> order = order_of(shuffler.shuffle(pud_test_node(4), children(9)));
    std::sort(order.begin(), order.end());
    EXPECT_EQ(order, (std::vector<uint32_t>{0, 1, 2, 3, 4, 5, 6, 7, 8}));
}

TEST_F(PudChildShufflerTest, TheSameParentAlwaysGetsTheSameOrder) {
    EXPECT_EQ(shuffler.shuffle(pud_test_node(4), children(9)), shuffler.shuffle(pud_test_node(4), children(9)));
}

TEST_F(PudChildShufflerTest, DifferentParentsGetDifferentOrders) {
    const std::vector<uint32_t> first = order_of(shuffler.shuffle(pud_test_node(4), children(9)));
    bool another_order = false;
    for (pud_node_id parent = 5; parent < 15; ++parent)
        another_order = another_order || order_of(shuffler.shuffle(pud_test_node(parent), children(9))) != first;
    EXPECT_TRUE(another_order);
}

TEST_F(PudChildShufflerTest, EachOfTwoChildrenComesFirstForSomeParent) {
    size_t kept = 0;
    const size_t parents = 200;
    for (pud_node_id parent = 0; parent < parents; ++parent)
        kept += order_of(shuffler.shuffle(pud_test_node(parent), children(2))).front() == 0 ? 1 : 0;
    EXPECT_GT(kept, parents / 4);
    EXPECT_LT(kept, parents * 3 / 4);
}
