#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_frontier_change_collector.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockCheckNodeRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_node_id), (const));
};

struct PudFrontierChangeCollectorTest : public ::testing::Test {
    NiceMock<MockCheckNodeRefuted> status;
    pud_frontier_change_collector<NiceMock<MockCheckNodeRefuted>> collector{status};

    void SetUp() override {
        ON_CALL(status, check_refuted(3)).WillByDefault(Return(true));
    }
};

}

TEST_F(PudFrontierChangeCollectorTest, LiveProducedNodesAreOffspringAndRefutedOnesAreOmitted) {
    EXPECT_EQ(collector.collect({1, 2, 3}, {3, 9}), (pud_frontier_change{.offspring = {1, 2}, .refuted = {3, 9}}));
}
