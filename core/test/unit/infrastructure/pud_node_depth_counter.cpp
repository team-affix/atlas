#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_depth_counter.hpp"

using ::testing::Return;

namespace {

pud_global_node_id node(pud_node_id id) {
    return pud_global_node_id{.module = 3, .node = id};
}

struct MockGetParent {
    MOCK_METHOD(std::optional<pud_global_node_id>, get_parent, (pud_global_node_id));
};

struct PudNodeDepthCounterTest : public ::testing::Test {
    MockGetParent parents;
    pud_node_depth_counter<MockGetParent> counter{parents};
};

}

TEST_F(PudNodeDepthCounterTest, AnAxiomIsDepthZero) {
    EXPECT_CALL(parents, get_parent(node(0))).WillOnce(Return(std::nullopt));
    EXPECT_EQ(counter.count(node(0)), 0u);
}

TEST_F(PudNodeDepthCounterTest, EachParentIsOneInferenceEdge) {
    EXPECT_CALL(parents, get_parent(node(3))).WillOnce(Return(node(2)));
    EXPECT_CALL(parents, get_parent(node(2))).WillOnce(Return(node(1)));
    EXPECT_CALL(parents, get_parent(node(1))).WillOnce(Return(std::nullopt));
    EXPECT_EQ(counter.count(node(3)), 2u);
}
