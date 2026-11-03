#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unfold_applier.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockUnfold {
    MOCK_METHOD(pud_frontier_change, unfold, (pud_global_node_id, pud_body_goal_id));
};
struct MockApplyUnfold {
    MOCK_METHOD(void, apply_unfold, (pud_global_node_id, const pud_frontier_change&));
};
struct MockExtendNodeDepths {
    MOCK_METHOD(void, extend, (pud_global_node_id, (const std::vector<pud_global_node_id>&)));
};
struct MockExtendFocus {
    MOCK_METHOD(void, extend, (pud_global_node_id, (const std::vector<pud_global_node_id>&)));
};
struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id), (const));
};

using test_applier_t = pud_unfold_applier<
    NiceMock<MockUnfold>, NiceMock<MockApplyUnfold>, NiceMock<MockExtendNodeDepths>, NiceMock<MockExtendFocus>,
    NiceMock<MockContainsFocusNode>>;

struct PudUnfoldApplierTest : public ::testing::Test {
    NiceMock<MockUnfold> unfolder;
    NiceMock<MockApplyUnfold> frontier;
    NiceMock<MockExtendNodeDepths> depth_extender;
    NiceMock<MockExtendFocus> focus_extender;
    NiceMock<MockContainsFocusNode> focus;
    test_applier_t applier{unfolder, frontier, depth_extender, focus_extender, focus};
    pud_frontier_change change{.offspring = {pud_test_node(10), pud_test_node(11)}, .refuted = {pud_test_node(2)}};

    void SetUp() override {
        ON_CALL(unfolder, unfold(pud_test_node(4), 1)).WillByDefault(Return(change));
    }
};

}

TEST_F(PudUnfoldApplierTest, TheChangeReachesTheFrontierThenTheNodeDepthsThenTheFocusSet) {
    InSequence order;
    EXPECT_CALL(unfolder, unfold(pud_test_node(4), 1));
    EXPECT_CALL(frontier, apply_unfold(pud_test_node(4), change));
    EXPECT_CALL(depth_extender, extend(pud_test_node(4), change.offspring));
    EXPECT_CALL(focus_extender, extend(pud_test_node(4), change.offspring));
    applier.unfold(pud_test_node(4), 1);
}

TEST_F(PudUnfoldApplierTest, AnUnfoldInsideTheConjectureModuleIsOnFocus) {
    ON_CALL(focus, contains(pud_test_node(4))).WillByDefault(Return(true));
    EXPECT_EQ(applier.unfold(pud_test_node(4), 1), pud_unfold_outcome::on_focus);
}

TEST_F(PudUnfoldApplierTest, AnUnfoldOutsideTheConjectureModuleIsOffFocus) {
    ON_CALL(focus, contains(pud_test_node(4))).WillByDefault(Return(false));
    EXPECT_EQ(applier.unfold(pud_test_node(4), 1), pud_unfold_outcome::off_focus);
}
