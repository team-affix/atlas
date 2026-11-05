#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_plain_node_deactivator.hpp"
#include "pud_test_node.hpp"

using ::testing::IsEmpty;
using ::testing::NiceMock;

namespace {

struct MockEraseNodeBindings {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};
struct MockEraseNodeLvc {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};

struct PudPlainNodeDeactivatorTest : public ::testing::Test {
    NiceMock<MockEraseNodeBindings> bindings;
    NiceMock<MockEraseNodeLvc> lvcs;
    pud_plain_node_deactivator<NiceMock<MockEraseNodeBindings>, NiceMock<MockEraseNodeLvc>> deactivator{
        bindings, lvcs};
};

}

TEST_F(PudPlainNodeDeactivatorTest, TheNodesBindingsAndLvcAreErased) {
    EXPECT_CALL(bindings, erase(pud_test_node(3)));
    EXPECT_CALL(lvcs, erase(pud_test_node(3)));
    deactivator.deactivate(pud_test_node(3));
}

TEST_F(PudPlainNodeDeactivatorTest, NoLeafIsStranded) {
    EXPECT_THAT(deactivator.deactivate(pud_test_node(3)), IsEmpty());
}
