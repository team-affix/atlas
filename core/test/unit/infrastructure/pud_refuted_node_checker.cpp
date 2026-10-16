#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_refuted_node_checker.hpp"

using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockGetLiveChildCount {
    MOCK_METHOD(size_t, get, (pud_node_id));
};

struct PudRefutedNodeCheckerTest : public ::testing::Test {
    StrictMock<MockGetLiveChildCount> get_live_child_count;
    pud_refuted_node_checker<StrictMock<MockGetLiveChildCount>> checker{get_live_child_count};
    pud_node_id node = 7;
};

TEST_F(PudRefutedNodeCheckerTest, NoLiveChildrenMeansRefuted) {
    EXPECT_CALL(get_live_child_count, get(node)).WillOnce(Return(0));
    EXPECT_TRUE(checker.check_refuted(node));
}

TEST_F(PudRefutedNodeCheckerTest, OneLiveChildMeansLive) {
    EXPECT_CALL(get_live_child_count, get(node)).WillOnce(Return(1));
    EXPECT_FALSE(checker.check_refuted(node));
}

TEST_F(PudRefutedNodeCheckerTest, ManyLiveChildrenMeansLive) {
    EXPECT_CALL(get_live_child_count, get(node)).WillOnce(Return(9));
    EXPECT_FALSE(checker.check_refuted(node));
}

}  // namespace
