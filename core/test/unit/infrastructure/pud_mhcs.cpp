#include <optional>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhcs.hpp"
#include "pud_test_node.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_replay.hpp"

using ::testing::_;
using ::testing::AllOf;
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
struct MockTryAddWitnessSearch {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_add, (pud_replay));
};
struct MockTryForkWitnessSearch {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_fork, (pud_mhws_head_id, pud_replay));
};
struct MockAdvanceWitnessSearch {
    MOCK_METHOD(std::optional<pud_witness_advance>, advance, (pud_mhws_head_id));
};
struct MockRemoveWitnessSearch {
    MOCK_METHOD(void, remove, (pud_mhws_head_id));
};
struct MockInvalidateWitnessLeaf {
    MOCK_METHOD(std::vector<pud_mhws_head_id>, invalidate_leaf, (pud_global_node_id));
};

std::optional<pud_replay> pud_enter_child(const pud_replay& parent, pud_global_node_id child) {
    return pud_replay{.node = child, .frame_offset = parent.frame_offset, .ubs_state = parent.ubs_state};
}

using test_mhcs_t = pud_mhcs<
    NiceMock<MockDescendReplay>, NiceMock<MockTryAddWitnessSearch>, NiceMock<MockTryForkWitnessSearch>,
    NiceMock<MockAdvanceWitnessSearch>, NiceMock<MockRemoveWitnessSearch>, NiceMock<MockInvalidateWitnessLeaf>,
    NiceMock<MockCheckNodeLeaf>, NiceMock<MockCheckNodeRefuted>, NiceMock<MockGetChildren>>;

struct PudMhcsTest : public ::testing::Test {
    NiceMock<MockDescendReplay> descend;
    NiceMock<MockCheckNodeLeaf> leaves;
    NiceMock<MockCheckNodeRefuted> refuted;
    NiceMock<MockGetChildren> children;
    std::unordered_map<pud_node_id, std::vector<pud_node_id>> child_lists;
    NiceMock<MockTryAddWitnessSearch> add_witness;
    NiceMock<MockTryForkWitnessSearch> fork_witness;
    NiceMock<MockAdvanceWitnessSearch> advance_witness;
    NiceMock<MockRemoveWitnessSearch> remove_witness;
    NiceMock<MockInvalidateWitnessLeaf> invalidate_witnesses;
    test_mhcs_t mhcs{
        descend, add_witness, fork_witness, advance_witness, remove_witness, invalidate_witnesses, leaves, refuted,
        children};

    void SetUp() override {
        ON_CALL(descend, descend(_, _)).WillByDefault(Invoke(&pud_enter_child));
        ON_CALL(leaves, check_leaf(_)).WillByDefault(Return(false));
        ON_CALL(refuted, check_refuted(_)).WillByDefault(Return(false));
    }
};

}

TEST_F(PudMhcsTest, ALeafAxiomRootIsItsOwnWitness) {
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}}).value();
    EXPECT_THAT(mhcs.path(head), ElementsAre(pud_test_node(1)));
    EXPECT_CALL(remove_witness, remove(_)).Times(0);
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ARootWithoutAWitnessIsNotAdmitted) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    EXPECT_FALSE(mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).has_value());
}

TEST_F(PudMhcsTest, ARefutedRootIsNotAdmitted) {
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_FALSE(mhcs.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}}).has_value());
}

TEST_F(PudMhcsTest, WitnessesUnderTwoDistinctChildrenMakeAChoicePoint) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(11u));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    EXPECT_THAT(mhcs.path(head), ElementsAre(pud_test_node(0)));
    EXPECT_CALL(remove_witness, remove(10u));
    EXPECT_CALL(remove_witness, remove(11u));
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ChildrenThatCannotBeEnteredOrWitnessedAreSkipped) {
    child_lists[0] = {1, 2, 3, 4};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    ON_CALL(descend, descend(_, pud_test_node(1))).WillByDefault(Return(std::nullopt));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(9u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(3)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(4)))).WillByDefault(Return(11u));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    EXPECT_CALL(remove_witness, remove(10u));
    EXPECT_CALL(remove_witness, remove(11u));
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ChildrenAreSteppedIntoInTheirParentsModule) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    EXPECT_CALL(
        descend,
        descend(Field(&pud_replay::node, pud_test_node(0)), pud_global_node_id{.module = k_test_module, .node = 1}));
    EXPECT_CALL(
        descend,
        descend(Field(&pud_replay::node, pud_test_node(0)), pud_global_node_id{.module = k_test_module, .node = 2}));
    mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}});
}

TEST_F(PudMhcsTest, ASoleWitnessStepsTheCandidateDownToTheNextChoicePoint) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {5, 6};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(6)))).WillByDefault(Return(12u));
    ON_CALL(advance_witness, advance(10u))
        .WillByDefault(Return(pud_witness_advance{
            .search_root = pud_replay{.node = pud_test_node(5), .frame_offset = 0u, .ubs_state = {}},
            .next_sibling = 1}));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    EXPECT_THAT(mhcs.path(head), ElementsAre(pud_test_node(0), pud_test_node(1)));
    EXPECT_CALL(remove_witness, remove(10u));
    EXPECT_CALL(remove_witness, remove(12u));
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ASoleWitnessStandingOnItsRootLeavesTheCandidateOnThatLeaf) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    EXPECT_THAT(mhcs.path(head), ElementsAre(pud_test_node(0), pud_test_node(1)));
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(1)), ElementsAre(head));
}

TEST_F(PudMhcsTest, ACandidateThatStepsOntoARefutedNodeReleasesItsWitness) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {5};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    ON_CALL(advance_witness, advance(10u))
        .WillByDefault(Return(pud_witness_advance{
            .search_root = pud_replay{.node = pud_test_node(5), .frame_offset = 0u, .ubs_state = {}},
            .next_sibling = 1}));
    EXPECT_CALL(remove_witness, remove(10u));
    EXPECT_FALSE(mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).has_value());
}

TEST_F(PudMhcsTest, AnUnfoldedSelfWitnessIsJustifiedAgainBelowIt) {
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}}).value();
    child_lists[1] = {2, 3};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(3)))).WillByDefault(Return(11u));
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(1)), IsEmpty());
    EXPECT_CALL(remove_witness, remove(10u));
    EXPECT_CALL(remove_witness, remove(11u));
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ARefutedSelfWitnessRefutesTheCandidate) {
    child_lists[1] = {};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(true));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(1), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(refuted, check_refuted(pud_test_node(1))).WillByDefault(Return(true));
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(1)), ElementsAre(head));
}

TEST_F(PudMhcsTest, InvalidatingALeafIsPassedOnToTheWitnessSearches) {
    EXPECT_CALL(invalidate_witnesses, invalidate_leaf(pud_test_node(3)));
    mhcs.invalidate_leaf(pud_test_node(3));
}

TEST_F(PudMhcsTest, AnExhaustedWitnessIsReplacedFromTheNextChild) {
    child_lists[0] = {1, 2, 3};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(11u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(3)))).WillByDefault(Return(12u));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(invalidate_witnesses, invalidate_leaf(_)).WillByDefault(Return(std::vector<pud_mhws_head_id>{10}));
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(7)), IsEmpty());
    EXPECT_CALL(remove_witness, remove(11u));
    EXPECT_CALL(remove_witness, remove(12u));
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ACandidateLearnsOfEveryExhaustedWitnessBeforeItResumes) {
    child_lists[0] = {1, 2, 3, 4};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(11u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(3)))).WillByDefault(Return(12u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(4)))).WillByDefault(Return(13u));
    mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}});
    ON_CALL(invalidate_witnesses, invalidate_leaf(_)).WillByDefault(Return(std::vector<pud_mhws_head_id>{10, 11}));
    EXPECT_CALL(add_witness, try_add(_)).Times(AnyNumber());
    EXPECT_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(3)))).Times(1);
    EXPECT_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(4)))).Times(1);
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(7)), IsEmpty());
}

TEST_F(PudMhcsTest, ExhaustingBothWitnessesWithNothingLeftRefutesTheCandidate) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(11u));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(invalidate_witnesses, invalidate_leaf(_)).WillByDefault(Return(std::vector<pud_mhws_head_id>{10, 11}));
    EXPECT_CALL(remove_witness, remove(_)).Times(0);
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(7)), ElementsAre(head));
}

TEST_F(PudMhcsTest, ExhaustedWitnessesReachOnlyTheirOwnCandidate) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[20] = {21, 22};
    ON_CALL(children, get_children(pud_test_node(20))).WillByDefault(ReturnRef(child_lists[20]));
    ON_CALL(leaves, check_leaf(pud_test_node(20))).WillByDefault(Return(false));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(11u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(21)))).WillByDefault(Return(30u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(22)))).WillByDefault(Return(31u));
    const pud_mhcs_head_id first =
        mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    const pud_mhcs_head_id second =
        mhcs.try_add(pud_replay{.node = pud_test_node(20), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(invalidate_witnesses, invalidate_leaf(_)).WillByDefault(Return(std::vector<pud_mhws_head_id>{10, 11}));
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(7)), ElementsAre(first));
    EXPECT_CALL(remove_witness, remove(30u));
    EXPECT_CALL(remove_witness, remove(31u));
    mhcs.remove(second);
}

TEST_F(PudMhcsTest, AForkReplaysThePathAndForksBothWitnessesOntoTheNewEnvironment) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {5, 6};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(6)))).WillByDefault(Return(12u));
    ON_CALL(advance_witness, advance(10u))
        .WillByDefault(Return(pud_witness_advance{
            .search_root = pud_replay{.node = pud_test_node(5), .frame_offset = 0u, .ubs_state = {}},
            .next_sibling = 1}));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    EXPECT_CALL(
        fork_witness,
        try_fork(10u, AllOf(Field(&pud_replay::node, pud_test_node(5)), Field(&pud_replay::frame_offset, 9u))))
        .WillOnce(Return(20u));
    EXPECT_CALL(
        fork_witness,
        try_fork(12u, AllOf(Field(&pud_replay::node, pud_test_node(6)), Field(&pud_replay::frame_offset, 9u))))
        .WillOnce(Return(21u));
    const pud_mhcs_head_id fork =
        mhcs.try_fork(head, pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}).value();
    EXPECT_THAT(mhcs.path(fork), ElementsAre(pud_test_node(0), pud_test_node(1)));
    EXPECT_CALL(remove_witness, remove(20u));
    EXPECT_CALL(remove_witness, remove(21u));
    mhcs.remove(fork);
}

TEST_F(PudMhcsTest, AForkWhosePathNoLongerReplaysIsNotAdmitted) {
    child_lists[0] = {1, 2};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    child_lists[1] = {5, 6};
    ON_CALL(children, get_children(pud_test_node(1))).WillByDefault(ReturnRef(child_lists[1]));
    ON_CALL(leaves, check_leaf(pud_test_node(1))).WillByDefault(Return(false));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(6)))).WillByDefault(Return(12u));
    ON_CALL(advance_witness, advance(10u))
        .WillByDefault(Return(pud_witness_advance{
            .search_root = pud_replay{.node = pud_test_node(5), .frame_offset = 0u, .ubs_state = {}},
            .next_sibling = 1}));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(descend, descend(Field(&pud_replay::frame_offset, 9u), pud_test_node(1)))
        .WillByDefault(Return(std::nullopt));
    EXPECT_CALL(fork_witness, try_fork(_, _)).Times(0);
    EXPECT_FALSE(
        mhcs.try_fork(head, pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}).has_value());
}

TEST_F(PudMhcsTest, AForkThatLosesAWitnessReplacesItFromTheRemainingChildren) {
    child_lists[0] = {1, 2, 3};
    ON_CALL(children, get_children(pud_test_node(0))).WillByDefault(ReturnRef(child_lists[0]));
    ON_CALL(leaves, check_leaf(pud_test_node(0))).WillByDefault(Return(false));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(1)))).WillByDefault(Return(10u));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(2)))).WillByDefault(Return(11u));
    const pud_mhcs_head_id head =
        mhcs.try_add(pud_replay{.node = pud_test_node(0), .frame_offset = 0u, .ubs_state = {}}).value();
    ON_CALL(fork_witness, try_fork(10u, _)).WillByDefault(Return(20u));
    ON_CALL(fork_witness, try_fork(11u, _)).WillByDefault(Return(std::nullopt));
    ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(3)))).WillByDefault(Return(22u));
    const pud_mhcs_head_id fork =
        mhcs.try_fork(head, pud_replay{.node = pud_test_node(0), .frame_offset = 9u, .ubs_state = {}}).value();
    EXPECT_CALL(remove_witness, remove(20u));
    EXPECT_CALL(remove_witness, remove(22u));
    mhcs.remove(fork);
}
