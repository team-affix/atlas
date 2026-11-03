#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_child_focus_recorder.hpp"
#include "pud_test_node.hpp"

using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id), (const));
};
struct MockInsertFocusNode {
    MOCK_METHOD(void, insert, (pud_global_node_id));
};
struct MockActivateChild {
    MOCK_METHOD(bool, activate_child, (pud_global_node_id, pud_global_node_id));
};

struct PudChildFocusRecorderTest : public ::testing::Test {
    StrictMock<MockContainsFocusNode> focus;
    StrictMock<MockInsertFocusNode> insert_focus;
    StrictMock<MockActivateChild> activator;
    pud_child_focus_recorder<
        StrictMock<MockContainsFocusNode>, StrictMock<MockInsertFocusNode>, StrictMock<MockActivateChild>>
        recorder{focus, insert_focus, activator};
};

}

TEST_F(PudChildFocusRecorderTest, AChildOfAFocusNodeJoinsTheFocusThenIsActivated) {
    InSequence order;
    EXPECT_CALL(focus, contains(pud_test_node(3))).WillOnce(Return(true));
    EXPECT_CALL(insert_focus, insert(pud_test_node(4)));
    EXPECT_CALL(activator, activate_child(pud_test_node(3), pud_test_node(4))).WillOnce(Return(false));
    EXPECT_FALSE(recorder.activate_child(pud_test_node(3), pud_test_node(4)));
}

TEST_F(PudChildFocusRecorderTest, AChildOfANodeOutsideTheFocusStaysOutside) {
    EXPECT_CALL(focus, contains(pud_test_node(3))).WillOnce(Return(false));
    EXPECT_CALL(activator, activate_child(pud_test_node(3), pud_test_node(4))).WillOnce(Return(true));
    EXPECT_TRUE(recorder.activate_child(pud_test_node(3), pud_test_node(4)));
}
