#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_focus_extender.hpp"

using ::testing::Return;

namespace {

pud_global_node_id node(pud_node_id id) {
    return pud_global_node_id{.module = 7, .node = id};
}

struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id));
};

struct MockInsertFocusNode {
    MOCK_METHOD(void, insert, (pud_global_node_id));
};

struct PudLodestarFocusExtenderTest : public ::testing::Test {
    MockContainsFocusNode contains;
    MockInsertFocusNode insert;
    pud_lodestar_focus_extender<MockContainsFocusNode, MockInsertFocusNode> extender{contains, insert};
};

}

TEST_F(PudLodestarFocusExtenderTest, OffspringOfANodeOutsideTheSetAreIgnored) {
    EXPECT_CALL(contains, contains(node(1))).WillOnce(Return(false));
    EXPECT_CALL(insert, insert).Times(0);
    extender.extend(node(1), {node(2), node(3)});
}

TEST_F(PudLodestarFocusExtenderTest, OffspringOfAFocusNodeJoinTheSet) {
    EXPECT_CALL(contains, contains(node(1))).WillOnce(Return(true));
    EXPECT_CALL(insert, insert(node(2)));
    EXPECT_CALL(insert, insert(node(3)));
    extender.extend(node(1), {node(2), node(3)});
}

TEST_F(PudLodestarFocusExtenderTest, AnEmptyOffspringListInsertsNothing) {
    EXPECT_CALL(contains, contains(node(1))).WillOnce(Return(true));
    EXPECT_CALL(insert, insert).Times(0);
    extender.extend(node(1), {});
}
