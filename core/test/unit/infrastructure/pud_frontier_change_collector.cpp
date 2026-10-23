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
    NiceMock<MockCheckNodeRefuted> checker;
    pud_frontier_change_collector<NiceMock<MockCheckNodeRefuted>> collector{checker};
};

}

TEST_F(PudFrontierChangeCollectorTest, ProducedNodesSkipRefutedOnesAndRefutedAreRecorded) {
    ON_CALL(checker, check_refuted(3)).WillByDefault(Return(true));
    collector.add_produced(1);
    collector.add_produced(2);
    collector.add_produced(3);
    collector.add_refuted(3);
    collector.add_refuted(9);
    EXPECT_EQ(collector.finish(), (pud_frontier_change{.offspring = {1, 2}, .refuted = {3, 9}}));
}
