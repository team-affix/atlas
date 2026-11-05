#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_trail_node_deactivator.hpp"
#include "pud_test_node.hpp"

using ::testing::IsEmpty;
using ::testing::NiceMock;

namespace {

struct MockEraseNodeTrails {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};
struct MockEraseNodeWatch {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};
struct MockEraseNodeBindings {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};
struct MockEraseNodeLvc {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};

struct PudTrailNodeDeactivatorTest : public ::testing::Test {
    NiceMock<MockEraseNodeTrails> trails;
    NiceMock<MockEraseNodeWatch> watches;
    NiceMock<MockEraseNodeBindings> bindings;
    NiceMock<MockEraseNodeLvc> lvcs;
    pud_trail_node_deactivator<
        NiceMock<MockEraseNodeTrails>, NiceMock<MockEraseNodeWatch>, NiceMock<MockEraseNodeBindings>,
        NiceMock<MockEraseNodeLvc>>
        deactivator{trails, watches, bindings, lvcs};
};

}

TEST_F(PudTrailNodeDeactivatorTest, EveryActiveFieldIsErased) {
    EXPECT_CALL(trails, erase(pud_test_node(3)));
    EXPECT_CALL(watches, erase(pud_test_node(3)));
    EXPECT_CALL(bindings, erase(pud_test_node(3)));
    EXPECT_CALL(lvcs, erase(pud_test_node(3)));
    deactivator.deactivate(pud_test_node(3));
}

TEST_F(PudTrailNodeDeactivatorTest, NoLeafIsStranded) {
    EXPECT_THAT(deactivator.deactivate(pud_test_node(3)), IsEmpty());
}
