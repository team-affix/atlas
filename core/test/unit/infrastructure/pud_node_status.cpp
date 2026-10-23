#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_node_status.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetNodeState {
    MOCK_METHOD(pud_node_state, get, (pud_node_id));
};
struct MockGetLiveChildCount {
    MOCK_METHOD(size_t, get, (pud_node_id));
};

struct PudNodeStatusTest : public ::testing::Test {
    NiceMock<MockGetNodeState> states;
    NiceMock<MockGetLiveChildCount> live_child_counts;
    pud_node_status<NiceMock<MockGetNodeState>, NiceMock<MockGetLiveChildCount>> status{states, live_child_counts};
};

}

TEST_F(PudNodeStatusTest, ALiveLeafIsALeafAndNotRefuted) {
    ON_CALL(states, get(1)).WillByDefault(Return(pud_node_state::leaf));
    EXPECT_TRUE(status.is_leaf(1));
    EXPECT_FALSE(status.is_refuted(1));
}

TEST_F(PudNodeStatusTest, ARefutedLeafIsStillALeaf) {
    ON_CALL(states, get(1)).WillByDefault(Return(pud_node_state::refuted));
    EXPECT_TRUE(status.is_leaf(1));
    EXPECT_TRUE(status.is_refuted(1));
}

TEST_F(PudNodeStatusTest, AnInteriorNodeIsRefutedExactlyWhenNoChildIsAlive) {
    ON_CALL(states, get(1)).WillByDefault(Return(pud_node_state::expanded));
    ON_CALL(live_child_counts, get(1)).WillByDefault(Return(2));
    EXPECT_FALSE(status.is_leaf(1));
    EXPECT_FALSE(status.is_refuted(1));
    ON_CALL(live_child_counts, get(1)).WillByDefault(Return(0));
    EXPECT_TRUE(status.is_refuted(1));
}

TEST_F(PudNodeStatusTest, ALeafsChildCountIsNeverRead) {
    ON_CALL(states, get(1)).WillByDefault(Return(pud_node_state::leaf));
    EXPECT_CALL(live_child_counts, get(::testing::_)).Times(0);
    status.is_refuted(1);
}
