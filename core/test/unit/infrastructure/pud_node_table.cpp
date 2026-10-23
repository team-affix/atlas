#include <optional>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_node_table.hpp"

namespace {

struct PudNodeTableTest : public ::testing::Test {
    pud_node_table<std::vector<pud_node_id>> table;
};

}

TEST_F(PudNodeTableTest, AStoredValueReadsBack) {
    table.store(0, {4, 5});
    EXPECT_EQ(table.get(0), (std::vector<pud_node_id>{4, 5}));
}

TEST_F(PudNodeTableTest, NodesMayBeStoredOutOfOrderAndSparsely) {
    table.store(7, {1});
    table.store(2, {2});
    EXPECT_EQ(table.get(7), (std::vector<pud_node_id>{1}));
    EXPECT_EQ(table.get(2), (std::vector<pud_node_id>{2}));
}

TEST_F(PudNodeTableTest, AStoredValueStaysInPlaceWhileLaterNodesAreStored) {
    table.store(0, {9});
    const std::vector<pud_node_id>& first = table.get(0);
    for (pud_node_id node = 1; node < 5000; ++node)
        table.store(node, {node});
    EXPECT_EQ(&table.get(0), &first);
    EXPECT_EQ(first, (std::vector<pud_node_id>{9}));
}

TEST_F(PudNodeTableTest, ReadingANodeWithoutAValueFails) {
    table.store(3, {1});
    EXPECT_THROW(table.get(1), std::bad_optional_access);
    EXPECT_THROW(table.get(10), std::out_of_range);
}
