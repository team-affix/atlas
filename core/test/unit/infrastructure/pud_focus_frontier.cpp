#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_focus_frontier.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

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
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id), (const));
};

using test_focus_frontier_t = pud_focus_frontier<
    NiceMock<MockApplyRegister>, NiceMock<MockApplyUnfold>, NiceMock<MockEraseRefuted>, NiceMock<MockEraseLeaf>,
    NiceMock<MockContainsFocusNode>, NiceMock<MockGetNodeGoals>>;

// Every node is in the focus except node 2. Every node has an open goal except nodes 2 and 3.
struct PudFocusFrontierTest : public ::testing::Test {
    PudFocusFrontierTest();
    NiceMock<MockApplyRegister> registrar;
    NiceMock<MockApplyUnfold> unfolder;
    NiceMock<MockEraseRefuted> refuted;
    NiceMock<MockEraseLeaf> eraser;
    NiceMock<MockContainsFocusNode> focus;
    NiceMock<MockGetNodeGoals> goals;
    test_focus_frontier_t frontier;
    pud_goal_list open;
    pud_goal_list closed;

    void SetUp() override {
        ON_CALL(focus, contains(_)).WillByDefault(Return(true));
        ON_CALL(focus, contains(pud_test_node(2))).WillByDefault(Return(false));
        ON_CALL(goals, get(_)).WillByDefault(ReturnRef(open));
        ON_CALL(goals, get(pud_test_node(2))).WillByDefault(ReturnRef(closed));
        ON_CALL(goals, get(pud_test_node(3))).WillByDefault(ReturnRef(closed));
    }
};

PudFocusFrontierTest::PudFocusFrontierTest()
    : registrar()
    , unfolder()
    , refuted()
    , eraser()
    , focus()
    , goals()
    , frontier(registrar, unfolder, refuted, eraser, focus, goals)
    , open(pud_goal_list{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(0, nullptr), .next_id = 1})
    , closed() {}

}

TEST_F(PudFocusFrontierTest, NothingIsLiveAtFirst) {
    EXPECT_TRUE(frontier.live().empty());
    EXPECT_TRUE(frontier.solved().empty());
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

TEST_F(PudFocusFrontierTest, OnlyLiveFocusLeavesWithoutOpenGoalsAreSolved) {
    frontier.apply_register(pud_frontier_change{
        .offspring = {pud_test_node(1), pud_test_node(2), pud_test_node(3)}, .refuted = {}});
    EXPECT_EQ(frontier.solved(), (std::set<pud_global_node_id>{pud_test_node(3)}));
}

TEST_F(PudFocusFrontierTest, ASolvedLeafLeavesTheSolvedSetWhenItLeavesTheFrontier) {
    frontier.apply_register(pud_frontier_change{.offspring = {pud_test_node(3)}, .refuted = {}});
    frontier.erase(pud_test_node(3));
    EXPECT_TRUE(frontier.solved().empty());
    frontier.apply_register(pud_frontier_change{.offspring = {pud_test_node(3)}, .refuted = {pud_test_node(3)}});
    EXPECT_TRUE(frontier.solved().empty());
    frontier.apply_register(pud_frontier_change{.offspring = {pud_test_node(3)}, .refuted = {}});
    frontier.erase_refuted({pud_test_node(3)});
    EXPECT_TRUE(frontier.solved().empty());
}

TEST_F(PudFocusFrontierTest, AnUnfoldsSolvedOffspringJoinTheSolvedSet) {
    frontier.apply_register(pud_frontier_change{.offspring = {pud_test_node(1)}, .refuted = {}});
    frontier.apply_unfold(pud_test_node(1), pud_frontier_change{.offspring = {pud_test_node(3)}, .refuted = {}});
    EXPECT_EQ(frontier.solved(), (std::set<pud_global_node_id>{pud_test_node(3)}));
}
