#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhws.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_replay.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockDescendReplay {
    MOCK_METHOD(std::optional<pud_replay>, descend, (const pud_replay&, pud_global_node_id));
};
struct MockCheckNodeLeaf {
    MOCK_METHOD(bool, check_leaf, (pud_global_node_id), (const));
};
struct MockCheckNodeRefuted {
    MOCK_METHOD(bool, check_refuted, (pud_global_node_id), (const));
};
struct MockGetChildren {
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id));
};

std::optional<pud_replay> pud_enter_child(const pud_replay& parent, pud_global_node_id child) {
    return pud_replay{.node = child, .frame_offset = parent.frame_offset, .ubs_state = parent.ubs_state};
}

using test_mhws_t = pud_mhws<
    NiceMock<MockDescendReplay>, NiceMock<MockCheckNodeLeaf>, NiceMock<MockCheckNodeRefuted>, NiceMock<MockGetChildren>>;

struct PudMhwsTest : public ::testing::Test {
    NiceMock<MockDescendReplay> descend;
    NiceMock<MockCheckNodeLeaf> leaves;
    NiceMock<MockCheckNodeRefuted> refuted;
    NiceMock<MockGetChildren> children;
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> child_lists;
    test_mhws_t mhws{descend, leaves, refuted, children};

    void SetUp() override {
        ON_CALL(descend, descend(_, _)).WillByDefault(Invoke(&pud_enter_child));
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(refuted, check_refuted(_)).WillByDefault(Return(false));
    }
};

}

TEST_F(PudMhwsTest, ALeafSearchRootIsItsOwnWitness) {
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    const std::optional<pud_mhws_head_id> head =
        mhws.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}});
    ASSERT_TRUE(head.has_value());
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(1)), ElementsAre(head.value()));
}

TEST_F(PudMhwsTest, ASearchThatReachesNoLeafIsNotAdmitted) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    child_lists[2] = {};
    ON_CALL(children, get_children(pud_test_node(2))).WillByDefault(ReturnRef(child_lists[2]));
    ON_CALL(leaves, check_leaf(pud_test_node(2))).WillByDefault(Return(true));
    ON_CALL(descend, descend(_, pud_test_node(2))).WillByDefault(Return(std::nullopt));
    EXPECT_FALSE(mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).has_value());
}

TEST_F(PudMhwsTest, ARefutedSearchRootIsNotAdmitted) {
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_FALSE(mhws.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}}).has_value());
}

TEST_F(PudMhwsTest, TheWitnessIsTheFirstLeafDepthFirstInChildOrder) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {3};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    child_lists[3] = {};
    ON_CALL(children, get_children(pud_test_node(3))).WillByDefault(ReturnRef(child_lists[3]));
    ON_CALL(leaves, check_leaf(pud_test_node(3))).WillByDefault(Return(true));
    child_lists[2] = {};
    ON_CALL(children, get_children(pud_test_node(2))).WillByDefault(ReturnRef(child_lists[2]));
    ON_CALL(leaves, check_leaf(pud_test_node(2))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(2)), IsEmpty());
    ON_CALL(refuted, check_refuted(pud_test_node(3))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(3)), IsEmpty());
    ON_CALL(refuted, check_refuted(pud_test_node(2))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(2)), ElementsAre(head));
}

TEST_F(PudMhwsTest, ChildrenAreSteppedIntoInTheirParentsModule) {
    child_lists[0] = {1};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_CALL(
        descend,
        descend(Field(&pud_replay::node, pud_test_node(0)), pud_global_node_id{.module = k_test_module, .node = 1}));
    mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}});
}

TEST_F(PudMhwsTest, BlockedAndRefutedChildrenAreSkipped) {
    child_lists[0] = {1, 2, 3};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    ON_CALL(descend, descend(_, pud_test_node(1))).WillByDefault(Return(std::nullopt));
    child_lists[2] = {};
    ON_CALL(children, get_children(pud_test_node(2))).WillByDefault(ReturnRef(child_lists[2]));
    ON_CALL(leaves, check_leaf(pud_test_node(2))).WillByDefault(Return(true));
    ON_CALL(refuted, check_refuted(pud_test_node(2))).WillByDefault(Return(true));
    child_lists[3] = {};
    ON_CALL(children, get_children(pud_test_node(3))).WillByDefault(ReturnRef(child_lists[3]));
    ON_CALL(leaves, check_leaf(pud_test_node(3))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(refuted, check_refuted(pud_test_node(3))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(3)), ElementsAre(head));
}

TEST_F(PudMhwsTest, ARefutedInteriorNodeHidesItsLeaves) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {3};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    child_lists[3] = {};
    ON_CALL(children, get_children(pud_test_node(3))).WillByDefault(ReturnRef(child_lists[3]));
    ON_CALL(leaves, check_leaf(pud_test_node(3))).WillByDefault(Return(true));
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    child_lists[2] = {};
    ON_CALL(children, get_children(pud_test_node(2))).WillByDefault(ReturnRef(child_lists[2]));
    ON_CALL(leaves, check_leaf(pud_test_node(2))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(refuted, check_refuted(pud_test_node(2))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(2)), ElementsAre(head));
}

TEST_F(PudMhwsTest, AnUnfoldedWitnessIsSearchedBelow) {
    child_lists[0] = {1};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    child_lists[1] = {4};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    child_lists[4] = {};
    ON_CALL(children, get_children(pud_test_node(4))).WillByDefault(ReturnRef(child_lists[4]));
    ON_CALL(leaves, check_leaf(pud_test_node(4))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(1)), IsEmpty());
    ON_CALL(refuted, check_refuted(pud_test_node(4))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(4)), ElementsAre(head));
}

TEST_F(PudMhwsTest, ADeadSubtreeIsLeftForTheParentsNextChild) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {3, 4};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    child_lists[3] = {};
    ON_CALL(children, get_children(pud_test_node(3))).WillByDefault(ReturnRef(child_lists[3]));
    ON_CALL(leaves, check_leaf(pud_test_node(3))).WillByDefault(Return(true));
    child_lists[4] = {};
    ON_CALL(children, get_children(pud_test_node(4))).WillByDefault(ReturnRef(child_lists[4]));
    ON_CALL(leaves, check_leaf(pud_test_node(4))).WillByDefault(Return(true));
    child_lists[2] = {};
    ON_CALL(children, get_children(pud_test_node(2))).WillByDefault(ReturnRef(child_lists[2]));
    ON_CALL(leaves, check_leaf(pud_test_node(2))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(refuted, check_refuted(pud_test_node(4))).WillByDefault(Return(true));
    ON_CALL(refuted, check_refuted(pud_test_node(3))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(3)), IsEmpty());
    ON_CALL(refuted, check_refuted(pud_test_node(2))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(2)), ElementsAre(head));
}

TEST_F(PudMhwsTest, AnExhaustedSearchIsReportedOnceAndDropped) {
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(1)), ElementsAre(head));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(1)), IsEmpty());
}

TEST_F(PudMhwsTest, InvalidatingAnUnwitnessedLeafChangesNothing) {
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    child_lists[2] = {};
    ON_CALL(children, get_children(pud_test_node(2))).WillByDefault(ReturnRef(child_lists[2]));
    ON_CALL(leaves, check_leaf(pud_test_node(2))).WillByDefault(Return(true));
    mhws.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}});
    ON_CALL(refuted, check_refuted(pud_test_node(2))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(2)), IsEmpty());
}

TEST_F(PudMhwsTest, EverySearchOnAnInvalidatedLeafResumes) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    child_lists[2] = {};
    ON_CALL(children, get_children(pud_test_node(2))).WillByDefault(ReturnRef(child_lists[2]));
    ON_CALL(leaves, check_leaf(pud_test_node(2))).WillByDefault(Return(true));
    const pud_mhws_head_id first =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    const pud_mhws_head_id second =
        mhws.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(1)), ElementsAre(second));
    ON_CALL(refuted, check_refuted(pud_test_node(2))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(2)), ElementsAre(first));
}

TEST_F(PudMhwsTest, ARemovedSearchIsNeverResumed) {
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    mhws.remove(mhws.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}}).value());
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(1)), IsEmpty());
}

TEST_F(PudMhwsTest, AdvanceStepsTheSearchRootOntoThePathAndReportsTheNextSibling) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {3};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    child_lists[3] = {};
    ON_CALL(children, get_children(pud_test_node(3))).WillByDefault(ReturnRef(child_lists[3]));
    ON_CALL(leaves, check_leaf(pud_test_node(3))).WillByDefault(Return(true));
    child_lists[2] = {};
    ON_CALL(children, get_children(pud_test_node(2))).WillByDefault(ReturnRef(child_lists[2]));
    ON_CALL(leaves, check_leaf(pud_test_node(2))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    EXPECT_EQ(
        mhws.advance(head),
        (pud_witness_advance{
            .search_root = pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}},
            .next_sibling = 1}));
    EXPECT_EQ(
        mhws.advance(head),
        (pud_witness_advance{
            .search_root = pud_replay{.node = pud_test_node(3), .frame_offset = 0u, .ubs_state = {}},
            .next_sibling = 1}));
}

TEST_F(PudMhwsTest, AdvanceKeepsTheSearchResumableBelowItsNewRoot) {
    child_lists[0] = {1};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {3, 4};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    child_lists[3] = {};
    ON_CALL(children, get_children(pud_test_node(3))).WillByDefault(ReturnRef(child_lists[3]));
    ON_CALL(leaves, check_leaf(pud_test_node(3))).WillByDefault(Return(true));
    child_lists[4] = {};
    ON_CALL(children, get_children(pud_test_node(4))).WillByDefault(ReturnRef(child_lists[4]));
    ON_CALL(leaves, check_leaf(pud_test_node(4))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    mhws.advance(head);
    ON_CALL(refuted, check_refuted(pud_test_node(3))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(3)), IsEmpty());
    ON_CALL(refuted, check_refuted(pud_test_node(4))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(4)), ElementsAre(head));
}

TEST_F(PudMhwsTest, AdvancingASearchStandingOnItsRootDropsIt) {
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}}).value();
    EXPECT_FALSE(mhws.advance(head).has_value());
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(1)), IsEmpty());
}

TEST_F(PudMhwsTest, AForkReplaysTheSearchPathFromItsNewRoot) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {3};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    child_lists[3] = {};
    ON_CALL(children, get_children(pud_test_node(3))).WillByDefault(ReturnRef(child_lists[3]));
    ON_CALL(leaves, check_leaf(pud_test_node(3))).WillByDefault(Return(true));
    child_lists[2] = {};
    ON_CALL(children, get_children(pud_test_node(2))).WillByDefault(ReturnRef(child_lists[2]));
    ON_CALL(leaves, check_leaf(pud_test_node(2))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    EXPECT_CALL(descend, descend(_, _)).Times(AnyNumber());
    EXPECT_CALL(descend, descend(Field(&pud_replay::frame_offset, 9u), pud_test_node(1)));
    EXPECT_CALL(descend, descend(Field(&pud_replay::frame_offset, 9u), pud_test_node(3)));
    const pud_mhws_head_id fork =
        mhws.try_fork(head, pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}).value();
    ON_CALL(refuted, check_refuted(pud_test_node(3))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(3)), IsEmpty());
    ON_CALL(refuted, check_refuted(pud_test_node(2))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(2)), ::testing::UnorderedElementsAre(head, fork));
}

TEST_F(PudMhwsTest, AForkThatCannotReplayItsPathMovesOnToLaterSiblings) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {3};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    child_lists[3] = {};
    ON_CALL(children, get_children(pud_test_node(3))).WillByDefault(ReturnRef(child_lists[3]));
    ON_CALL(leaves, check_leaf(pud_test_node(3))).WillByDefault(Return(true));
    child_lists[2] = {};
    ON_CALL(children, get_children(pud_test_node(2))).WillByDefault(ReturnRef(child_lists[2]));
    ON_CALL(leaves, check_leaf(pud_test_node(2))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(descend, descend(Field(&pud_replay::frame_offset, 9u), pud_test_node(1)))
        .WillByDefault(::testing::Return(std::nullopt));
    const pud_mhws_head_id fork =
        mhws.try_fork(head, pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}).value();
    ON_CALL(refuted, check_refuted(pud_test_node(2))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(2)), ElementsAre(fork));
    ON_CALL(refuted, check_refuted(pud_test_node(3))).WillByDefault(Return(true));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(3)), ElementsAre(head));
}

TEST_F(PudMhwsTest, AForkThatReachesNoLeafIsNotAdmitted) {
    child_lists[0] = {1};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    const pud_mhws_head_id head =
        mhws.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(descend, descend(Field(&pud_replay::frame_offset, 9u), _)).WillByDefault(::testing::Return(std::nullopt));
    EXPECT_FALSE(mhws.try_fork(head, pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}).has_value());
}
