#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_root_depth_recorder.hpp"
#include "pud_test_node.hpp"

using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockStoreNodeDepth {
    MOCK_METHOD(void, store, (pud_global_node_id, size_t));
};
struct MockActivateRoot {
    MOCK_METHOD(bool, activate_root, (pud_global_node_id));
};

struct PudRootDepthRecorderTest : public ::testing::Test {
    StrictMock<MockStoreNodeDepth> depths;
    StrictMock<MockActivateRoot> activator;
    pud_root_depth_recorder<StrictMock<MockStoreNodeDepth>, StrictMock<MockActivateRoot>> recorder{depths, activator};
};

}

TEST_F(PudRootDepthRecorderTest, ARootIsRecordedAtDepthZeroThenActivated) {
    InSequence order;
    EXPECT_CALL(depths, store(pud_test_node(2), 0u));
    EXPECT_CALL(activator, activate_root(pud_test_node(2))).WillOnce(Return(false));
    EXPECT_FALSE(recorder.activate_root(pud_test_node(2)));
}

TEST_F(PudRootDepthRecorderTest, TheActivationResultIsPassedBack) {
    EXPECT_CALL(depths, store(pud_test_node(2), 0u));
    EXPECT_CALL(activator, activate_root(pud_test_node(2))).WillOnce(Return(true));
    EXPECT_TRUE(recorder.activate_root(pud_test_node(2)));
}
