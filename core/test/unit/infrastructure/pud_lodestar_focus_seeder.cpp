#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_focus_seeder.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

namespace {

pud_global_node_id node(pud_node_id id) {
    return pud_global_node_id{.module = 7, .node = id};
}

struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};

struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id));
};

struct MockInsertFocusNode {
    MOCK_METHOD(void, insert, (pud_global_node_id));
};

struct PudLodestarFocusSeederTest : public ::testing::Test {
    MockGetChildren children;
    MockContainsFocusNode contains;
    MockInsertFocusNode insert;
    pud_lodestar_focus_seeder<MockGetChildren, MockContainsFocusNode, MockInsertFocusNode> seeder{
        node(0), children, contains, insert};
    std::vector<pud_node_id> none;
    std::vector<pud_node_id> axiom_children{1, 2};
    std::vector<pud_node_id> first_child{3};
};

}

TEST_F(PudLodestarFocusSeederTest, AnotherModuleIsNotWalked) {
    EXPECT_CALL(children, get_children).Times(0);
    seeder.seed(4);
}

TEST_F(PudLodestarFocusSeederTest, ChildrenOfTheFocusAxiomAreInsertedAndWalked) {
    EXPECT_CALL(children, get_children(node(0))).WillOnce(ReturnRef(axiom_children));
    EXPECT_CALL(children, get_children(node(1))).WillOnce(ReturnRef(first_child));
    EXPECT_CALL(children, get_children(node(2))).WillOnce(ReturnRef(none));
    EXPECT_CALL(children, get_children(node(3))).WillOnce(ReturnRef(none));
    EXPECT_CALL(contains, contains).WillRepeatedly(Return(false));
    EXPECT_CALL(insert, insert(node(1)));
    EXPECT_CALL(insert, insert(node(2)));
    EXPECT_CALL(insert, insert(node(3)));
    seeder.seed(7);
}

TEST_F(PudLodestarFocusSeederTest, AChildAlreadyInTheSetIsNotInsertedOrDescended) {
    EXPECT_CALL(children, get_children(node(0))).WillOnce(ReturnRef(axiom_children));
    EXPECT_CALL(children, get_children(node(2))).WillOnce(ReturnRef(none));
    EXPECT_CALL(contains, contains(node(1))).WillOnce(Return(true));
    EXPECT_CALL(contains, contains(node(2))).WillOnce(Return(false));
    EXPECT_CALL(insert, insert(node(2)));
    seeder.seed(7);
}
