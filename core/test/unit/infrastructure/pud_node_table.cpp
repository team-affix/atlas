#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_node_table.hpp"
#include "pud_test_node.hpp"

namespace {

struct PudNodeTableTest : public ::testing::Test {
    pud_node_table<std::vector<pud_node_id>> table;
};

}

TEST_F(PudNodeTableTest, AStoredValueReadsBack) {
    table.store(pud_test_node(0), {4, 5});
    EXPECT_EQ(table.get(pud_test_node(0)), (std::vector<pud_node_id>{4, 5}));
}

TEST_F(PudNodeTableTest, NodesMayBeStoredOutOfOrderAndSparsely) {
    table.store(pud_test_node(7), {1});
    table.store(pud_test_node(2), {2});
    EXPECT_EQ(table.get(pud_test_node(7)), (std::vector<pud_node_id>{1}));
    EXPECT_EQ(table.get(pud_test_node(2)), (std::vector<pud_node_id>{2}));
}

TEST_F(PudNodeTableTest, AStoredValueStaysInPlaceWhileLaterNodesAreStored) {
    table.store(pud_test_node(0), {9});
    const std::vector<pud_node_id>& first = table.get(pud_test_node(0));
    for (pud_node_id node = 1; node < 5000; ++node)
        table.store(pud_test_node(node), {node});
    EXPECT_EQ(&table.get(pud_test_node(0)), &first);
    EXPECT_EQ(first, (std::vector<pud_node_id>{9}));
}

TEST_F(PudNodeTableTest, ReadingANodeWithoutAValueFails) {
    table.store(pud_test_node(3), {1});
    EXPECT_THROW(table.get(pud_test_node(1)), std::out_of_range);
    EXPECT_THROW(table.get(pud_test_node(10)), std::out_of_range);
}

TEST_F(PudNodeTableTest, TheSameLocalIdInTwoModulesIsTwoDistinctEntries) {
    const pud_global_node_id in_test_module = pud_test_node(3);
    const pud_global_node_id in_other_module{.module = k_test_module + 1, .node = 3};
    table.store(in_test_module, {1});
    table.store(in_other_module, {2});
    EXPECT_EQ(table.get(in_test_module), (std::vector<pud_node_id>{1}));
    EXPECT_EQ(table.get(in_other_module), (std::vector<pud_node_id>{2}));
}
