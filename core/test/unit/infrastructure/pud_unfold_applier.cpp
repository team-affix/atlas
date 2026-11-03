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
struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id), (const));
};

using test_applier_t = pud_unfold_applier<
    NiceMock<MockUnfold>, NiceMock<MockApplyUnfold>, NiceMock<MockContainsFocusNode>>;

struct PudUnfoldApplierTest : public ::testing::Test {
    NiceMock<MockUnfold> unfolder;
    NiceMock<MockApplyUnfold> frontier;
    NiceMock<MockContainsFocusNode> focus;
    test_applier_t applier{unfolder, frontier, focus};
    pud_frontier_change change{.offspring = {pud_test_node(10), pud_test_node(11)}, .refuted = {pud_test_node(2)}};

    void SetUp() override {
        ON_CALL(unfolder, unfold(pud_test_node(4), 1)).WillByDefault(Return(change));
    }
};

}

TEST_F(PudUnfoldApplierTest, TheUnfoldsChangeReachesTheFrontier) {
    InSequence order;
    EXPECT_CALL(unfolder, unfold(pud_test_node(4), 1));
    EXPECT_CALL(frontier, apply_unfold(pud_test_node(4), change));
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
