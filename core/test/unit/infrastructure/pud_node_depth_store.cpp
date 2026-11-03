#include <stdexcept>
#include <gtest/gtest.h>
#include "infrastructure/pud_node_depth_store.hpp"
#include "pud_test_node.hpp"

namespace {

struct PudNodeDepthStoreTest : public ::testing::Test {
    pud_node_depth_store depths;
};

}

TEST_F(PudNodeDepthStoreTest, AnInsertedDepthIsReadBack) {
    depths.insert(pud_test_node(3), 2);
    depths.insert(pud_test_node(4), 7);
    EXPECT_EQ(depths.get_depth(pud_test_node(3)), 2u);
    EXPECT_EQ(depths.get_depth(pud_test_node(4)), 7u);
}

TEST_F(PudNodeDepthStoreTest, NodesInDifferentModulesAreKeptApart) {
    depths.insert(pud_global_node_id{.module = 1, .node = 3}, 1);
    depths.insert(pud_global_node_id{.module = 2, .node = 3}, 5);
    EXPECT_EQ(depths.get_depth(pud_global_node_id{.module = 1, .node = 3}), 1u);
    EXPECT_EQ(depths.get_depth(pud_global_node_id{.module = 2, .node = 3}), 5u);
}

TEST_F(PudNodeDepthStoreTest, ANodeWithoutADepthIsAnError) {
    EXPECT_THROW((void)depths.get_depth(pud_test_node(3)), std::out_of_range);
}
