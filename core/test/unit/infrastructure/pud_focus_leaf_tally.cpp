#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_focus_leaf_tally.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::Assign;
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
    MOCK_METHOD(void, erase_refuted, (const std::vector<pud_global_node_id>&));
};
struct MockEraseLeaf {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};
struct MockGetLiveNodes {
    MOCK_METHOD(const std::set<pud_global_node_id>&, live, (), (const));
};
struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id), (const));
};
struct MockAllowUnfold {
    MOCK_METHOD(bool, allow, (pud_global_node_id), (const));
};

using test_tally_t = pud_focus_leaf_tally<
    NiceMock<MockApplyRegister>, NiceMock<MockApplyUnfold>, NiceMock<MockEraseRefuted>, NiceMock<MockEraseLeaf>,
    NiceMock<MockGetLiveNodes>, NiceMock<MockContainsFocusNode>, NiceMock<MockAllowUnfold>>;

// The live set stands in for the frontier: each forwarded mutation assigns the set it would leave behind.
// Node 9 is outside the focus; no unfold is allowed on node 5.
struct PudFocusLeafTallyTest : public ::testing::Test {
    PudFocusLeafTallyTest();
    void register_live(const std::set<pud_global_node_id>& after, const pud_frontier_change& change);
    NiceMock<MockApplyRegister> registerer;
    NiceMock<MockApplyUnfold> unfolder;
    NiceMock<MockEraseRefuted> refuted_eraser;
    NiceMock<MockEraseLeaf> leaf_eraser;
    NiceMock<MockGetLiveNodes> frontier;
    NiceMock<MockContainsFocusNode> focus;
    NiceMock<MockAllowUnfold> allower;
    test_tally_t tally{registerer, unfolder, refuted_eraser, leaf_eraser, frontier, focus, allower};
    std::set<pud_global_node_id> live;
};

PudFocusLeafTallyTest::PudFocusLeafTallyTest()
    : live() {
    ON_CALL(frontier, live()).WillByDefault(ReturnRef(live));
    ON_CALL(focus, contains(_)).WillByDefault(Return(true));
    ON_CALL(focus, contains(pud_test_node(9))).WillByDefault(Return(false));
    ON_CALL(allower, allow(_)).WillByDefault(Return(true));
    ON_CALL(allower, allow(pud_test_node(5))).WillByDefault(Return(false));
}

void PudFocusLeafTallyTest::register_live(
    const std::set<pud_global_node_id>& after, const pud_frontier_change& change) {
    EXPECT_CALL(registerer, apply_register(change)).WillOnce(Assign(&live, after));
    tally.apply_register(change);
}

}

TEST_F(PudFocusLeafTallyTest, NothingIsCountedAtFirst) {
    EXPECT_EQ(tally.count(), (pud_focus_leaf_counts{.live = 0, .blocked = 0}));
}

TEST_F(PudFocusLeafTallyTest, RegisteredOffspringUnderTheFocusAreCountedAndBlockedOnesAlsoAsBlocked) {
    register_live(
        {pud_test_node(1), pud_test_node(5), pud_test_node(9)},
        pud_frontier_change{.offspring = {pud_test_node(1), pud_test_node(5), pud_test_node(9)}, .refuted = {}});
    EXPECT_EQ(tally.count(), (pud_focus_leaf_counts{.live = 2, .blocked = 1}));
}

TEST_F(PudFocusLeafTallyTest, OffspringRefutedOnArrivalAreNotCounted) {
    register_live(
        {pud_test_node(1)},
        pud_frontier_change{.offspring = {pud_test_node(1), pud_test_node(2)}, .refuted = {pud_test_node(2)}});
    EXPECT_EQ(tally.count(), (pud_focus_leaf_counts{.live = 1, .blocked = 0}));
}

TEST_F(PudFocusLeafTallyTest, AnUnfoldReplacesTheParentWithItsLiveOffspring) {
    register_live({pud_test_node(5)}, pud_frontier_change{.offspring = {pud_test_node(5)}, .refuted = {}});
    const pud_frontier_change change{
        .offspring = {pud_test_node(2), pud_test_node(3), pud_test_node(4)}, .refuted = {pud_test_node(4)}};
    EXPECT_CALL(unfolder, apply_unfold(pud_test_node(5), change))
        .WillOnce(Assign(&live, std::set<pud_global_node_id>{pud_test_node(2), pud_test_node(3)}));
    tally.apply_unfold(pud_test_node(5), change);
    EXPECT_EQ(tally.count(), (pud_focus_leaf_counts{.live = 2, .blocked = 0}));
}

TEST_F(PudFocusLeafTallyTest, AParentThatIsAlsoRefutedLeavesTheCountOnce) {
    register_live(
        {pud_test_node(1), pud_test_node(2)},
        pud_frontier_change{.offspring = {pud_test_node(1), pud_test_node(2)}, .refuted = {}});
    const pud_frontier_change change{.offspring = {pud_test_node(3)}, .refuted = {pud_test_node(3), pud_test_node(1)}};
    EXPECT_CALL(unfolder, apply_unfold(pud_test_node(1), change))
        .WillOnce(Assign(&live, std::set<pud_global_node_id>{pud_test_node(2)}));
    tally.apply_unfold(pud_test_node(1), change);
    EXPECT_EQ(tally.count(), (pud_focus_leaf_counts{.live = 1, .blocked = 0}));
}

TEST_F(PudFocusLeafTallyTest, ErasingRefutedLeavesTakesOnlyTheLiveOnesOut) {
    register_live(
        {pud_test_node(1), pud_test_node(5)},
        pud_frontier_change{.offspring = {pud_test_node(1), pud_test_node(5)}, .refuted = {}});
    const std::vector<pud_global_node_id> refuted{pud_test_node(5), pud_test_node(7)};
    EXPECT_CALL(refuted_eraser, erase_refuted(refuted))
        .WillOnce(Assign(&live, std::set<pud_global_node_id>{pud_test_node(1)}));
    tally.erase_refuted(refuted);
    EXPECT_EQ(tally.count(), (pud_focus_leaf_counts{.live = 1, .blocked = 0}));
}

TEST_F(PudFocusLeafTallyTest, ErasingALeafTakesItOut) {
    register_live(
        {pud_test_node(1), pud_test_node(5)},
        pud_frontier_change{.offspring = {pud_test_node(1), pud_test_node(5)}, .refuted = {}});
    EXPECT_CALL(leaf_eraser, erase(pud_test_node(5)))
        .WillOnce(Assign(&live, std::set<pud_global_node_id>{pud_test_node(1)}));
    tally.erase(pud_test_node(5));
    EXPECT_EQ(tally.count(), (pud_focus_leaf_counts{.live = 1, .blocked = 0}));
}

TEST_F(PudFocusLeafTallyTest, ErasingANodeThatIsNotLiveChangesNothing) {
    register_live({pud_test_node(1)}, pud_frontier_change{.offspring = {pud_test_node(1)}, .refuted = {}});
    EXPECT_CALL(leaf_eraser, erase(pud_test_node(8)));
    tally.erase(pud_test_node(8));
    EXPECT_EQ(tally.count(), (pud_focus_leaf_counts{.live = 1, .blocked = 0}));
}
