#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_child_depth_recorder.hpp"
#include "pud_test_node.hpp"

using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockGetNodeDepth {
    MOCK_METHOD(size_t, get, (pud_global_node_id), (const));
};
struct MockStoreNodeDepth {
    MOCK_METHOD(void, store, (pud_global_node_id, size_t));
};
struct MockActivateChild {
    MOCK_METHOD(bool, activate_child, (pud_global_node_id, pud_global_node_id));
};

struct PudChildDepthRecorderTest : public ::testing::Test {
    StrictMock<MockGetNodeDepth> get_depths;
    StrictMock<MockStoreNodeDepth> store_depths;
    StrictMock<MockActivateChild> activator;
    pud_child_depth_recorder<
        StrictMock<MockGetNodeDepth>, StrictMock<MockStoreNodeDepth>, StrictMock<MockActivateChild>>
        recorder{get_depths, store_depths, activator};
};

}

TEST_F(PudChildDepthRecorderTest, AChildIsRecordedOneDeeperThanItsParentThenActivated) {
    InSequence order;
    EXPECT_CALL(get_depths, get(pud_test_node(3))).WillOnce(Return(6u));
    EXPECT_CALL(store_depths, store(pud_test_node(4), 7u));
    EXPECT_CALL(activator, activate_child(pud_test_node(3), pud_test_node(4))).WillOnce(Return(false));
    EXPECT_FALSE(recorder.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildDepthRecorderTest, TheActivationResultIsPassedBack) {
    EXPECT_CALL(get_depths, get(pud_test_node(3))).WillOnce(Return(0u));
    EXPECT_CALL(store_depths, store(pud_test_node(4), 1u));
    EXPECT_CALL(activator, activate_child(pud_test_node(3), pud_test_node(4))).WillOnce(Return(true));
    EXPECT_TRUE(recorder.activate_child(pud_test_node(3), pud_test_node(4)));
}
