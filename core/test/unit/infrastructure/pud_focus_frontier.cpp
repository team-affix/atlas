#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_focus_frontier.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockApplyRegister {
    MOCK_METHOD(void, apply_register, (const pud_frontier_change&));
};
struct MockApplyUnfold {
    MOCK_METHOD(void, apply_unfold, (pud_global_node_id, const pud_frontier_change&));
};
struct MockEraseRefuted {
    MOCK_METHOD(void, erase_refuted, ((const std::vector<pud_global_node_id>&)));
};
struct MockEraseLeaf {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};
struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id), (const));
};

using test_focus_frontier_t = pud_focus_frontier<
    NiceMock<MockApplyRegister>, NiceMock<MockApplyUnfold>, NiceMock<MockEraseRefuted>, NiceMock<MockEraseLeaf>,
    NiceMock<MockContainsFocusNode>>;

// Every node is in the focus except node 2.
struct PudFocusFrontierTest : public ::testing::Test {
    NiceMock<MockApplyRegister> registrar;
    NiceMock<MockApplyUnfold> unfolder;
    NiceMock<MockEraseRefuted> refuted;
    NiceMock<MockEraseLeaf> eraser;
    NiceMock<MockContainsFocusNode> focus;
    test_focus_frontier_t frontier{registrar, unfolder, refuted, eraser, focus};

    void SetUp() override {
        ON_CALL(focus, contains(_)).WillByDefault(Return(true));
        ON_CALL(focus, contains(pud_test_node(2))).WillByDefault(Return(false));
    }
};

}

TEST_F(PudFocusFrontierTest, NothingIsLiveAtFirst) {
    EXPECT_TRUE(frontier.live().empty());
}

TEST_F(PudFocusFrontierTest, ARegisterIsForwardedAndOnlyItsOffspringUnderTheFocusGoLive) {
    const pud_frontier_change change{
        .offspring = {pud_test_node(1), pud_test_node(2), pud_test_node(3)}, .refuted = {}};
    EXPECT_CALL(registrar, apply_register(change));
    frontier.apply_register(change);
    EXPECT_EQ(frontier.live(), (std::set<pud_global_node_id>{pud_test_node(1), pud_test_node(3)}));
}

TEST_F(PudFocusFrontierTest, OffspringARegisterRefutesAreNotLive) {
    frontier.apply_register(pud_frontier_change{
        .offspring = {pud_test_node(1), pud_test_node(3)}, .refuted = {pud_test_node(3)}});
    EXPECT_EQ(frontier.live(), (std::set<pud_global_node_id>{pud_test_node(1)}));
}

TEST_F(PudFocusFrontierTest, AnUnfoldIsForwardedRetiresTheParentAndKeepsItsUnrefutedFocusOffspring) {
    frontier.apply_register(pud_frontier_change{.offspring = {pud_test_node(1), pud_test_node(3)}, .refuted = {}});
    const pud_frontier_change change{
        .offspring = {pud_test_node(2), pud_test_node(4), pud_test_node(5)}, .refuted = {pud_test_node(5)}};
    EXPECT_CALL(unfolder, apply_unfold(pud_test_node(1), change));
    frontier.apply_unfold(pud_test_node(1), change);
    EXPECT_EQ(frontier.live(), (std::set<pud_global_node_id>{pud_test_node(3), pud_test_node(4)}));
}

TEST_F(PudFocusFrontierTest, ErasingRefutedLeavesIsForwardedAndTakesThemOut) {
    frontier.apply_register(pud_frontier_change{.offspring = {pud_test_node(1), pud_test_node(3)}, .refuted = {}});
    const std::vector<pud_global_node_id> refuted_leaves{pud_test_node(3), pud_test_node(2)};
    EXPECT_CALL(refuted, erase_refuted(refuted_leaves));
    frontier.erase_refuted(refuted_leaves);
    EXPECT_EQ(frontier.live(), (std::set<pud_global_node_id>{pud_test_node(1)}));
}

TEST_F(PudFocusFrontierTest, ErasingALeafIsForwardedAndTakesItOut) {
    frontier.apply_register(pud_frontier_change{.offspring = {pud_test_node(1), pud_test_node(3)}, .refuted = {}});
    EXPECT_CALL(eraser, erase(pud_test_node(1)));
    frontier.erase(pud_test_node(1));
    EXPECT_EQ(frontier.live(), (std::set<pud_global_node_id>{pud_test_node(3)}));
}
