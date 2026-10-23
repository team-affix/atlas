#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhcs.hpp"
#include "pud_search_tree.hpp"

using ::testing::_;
using ::testing::AllOf;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

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

using test_mhcs_t = pud_mhcs<
    NiceMock<MockDescendReplay>, NiceMock<MockTryAddWitnessSearch>, NiceMock<MockTryForkWitnessSearch>,
    NiceMock<MockAdvanceWitnessSearch>, NiceMock<MockRemoveWitnessSearch>, NiceMock<MockInvalidateWitnessLeaf>,
    NiceMock<MockCheckNodeLeaf>, NiceMock<MockCheckNodeRefuted>, NiceMock<MockGetChildren>>;

struct PudMhcsTest : public ::testing::Test {
    pud_search_tree tree;
    NiceMock<MockTryAddWitnessSearch> add_witness;
    NiceMock<MockTryForkWitnessSearch> fork_witness;
    NiceMock<MockAdvanceWitnessSearch> advance_witness;
    NiceMock<MockRemoveWitnessSearch> remove_witness;
    NiceMock<MockInvalidateWitnessLeaf> invalidate_witnesses;
    test_mhcs_t mhcs{
        tree.descend, add_witness, fork_witness, advance_witness, remove_witness, invalidate_witnesses,
        tree.leaves, tree.refuted, tree.children};

    void witnessed_under(pud_node_id search_root, pud_mhws_head_id witness) {
        ON_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(search_root)))).WillByDefault(Return(witness));
    }

    void exhaust(const std::vector<pud_mhws_head_id>& witnesses) {
        ON_CALL(invalidate_witnesses, invalidate_leaf(_)).WillByDefault(Return(witnesses));
    }

    // Root 0 has children 1 and 2; only 1 is witnessed, so the candidate steps onto 1, whose children 5
    // and 6 carry the two witnesses 10 and 12 of the choice point it settles on.
    pud_mhcs_head_id add_stepped_choice_point() {
        tree.branch(0, {1, 2});
        tree.branch(1, {5, 6});
        witnessed_under(1, 10);
        witnessed_under(6, 12);
        ON_CALL(advance_witness, advance(10u))
            .WillByDefault(Return(pud_witness_advance{.search_root = pud_replay_at(5, 0), .next_sibling = 1}));
        return mhcs.try_add(pud_replay_at(0, 0)).value();
    }
};

}

TEST_F(PudMhcsTest, ALeafAxiomRootIsItsOwnWitness) {
    tree.leaf(1);
    const pud_mhcs_head_id head = mhcs.try_add(pud_replay_at(1, 0)).value();
    EXPECT_THAT(mhcs.path(head), ElementsAre(pud_test_node(1)));
    EXPECT_CALL(remove_witness, remove(_)).Times(0);
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ARootWithoutAWitnessIsNotAdmitted) {
    tree.branch(0, {1, 2});
    EXPECT_FALSE(mhcs.try_add(pud_replay_at(0, 0)).has_value());
}

TEST_F(PudMhcsTest, ARefutedRootIsNotAdmitted) {
    tree.leaf(1);
    tree.refute(1);
    EXPECT_FALSE(mhcs.try_add(pud_replay_at(1, 0)).has_value());
}

TEST_F(PudMhcsTest, WitnessesUnderTwoDistinctChildrenMakeAChoicePoint) {
    tree.branch(0, {1, 2});
    witnessed_under(1, 10);
    witnessed_under(2, 11);
    const pud_mhcs_head_id head = mhcs.try_add(pud_replay_at(0, 0)).value();
    EXPECT_THAT(mhcs.path(head), ElementsAre(pud_test_node(0)));
    EXPECT_CALL(remove_witness, remove(10u));
    EXPECT_CALL(remove_witness, remove(11u));
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ChildrenThatCannotBeEnteredOrWitnessedAreSkipped) {
    tree.branch(0, {1, 2, 3, 4});
    tree.block(1);
    witnessed_under(1, 9);
    witnessed_under(3, 10);
    witnessed_under(4, 11);
    const pud_mhcs_head_id head = mhcs.try_add(pud_replay_at(0, 0)).value();
    EXPECT_CALL(remove_witness, remove(10u));
    EXPECT_CALL(remove_witness, remove(11u));
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ChildrenAreSteppedIntoInTheirParentsModule) {
    tree.branch(0, {1, 2});
    EXPECT_CALL(
        tree.descend,
        descend(Field(&pud_replay::node, pud_test_node(0)), pud_global_node_id{.module = k_test_module, .node = 1}));
    EXPECT_CALL(
        tree.descend,
        descend(Field(&pud_replay::node, pud_test_node(0)), pud_global_node_id{.module = k_test_module, .node = 2}));
    mhcs.try_add(pud_replay_at(0, 0));
}

TEST_F(PudMhcsTest, ASoleWitnessStepsTheCandidateDownToTheNextChoicePoint) {
    const pud_mhcs_head_id head = add_stepped_choice_point();
    EXPECT_THAT(mhcs.path(head), ElementsAre(pud_test_node(0), pud_test_node(1)));
    EXPECT_CALL(remove_witness, remove(10u));
    EXPECT_CALL(remove_witness, remove(12u));
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ASoleWitnessStandingOnItsRootLeavesTheCandidateOnThatLeaf) {
    tree.branch(0, {1, 2});
    tree.leaf(1);
    witnessed_under(1, 10);
    const pud_mhcs_head_id head = mhcs.try_add(pud_replay_at(0, 0)).value();
    EXPECT_THAT(mhcs.path(head), ElementsAre(pud_test_node(0), pud_test_node(1)));
    tree.refute(1);
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(1)), ElementsAre(head));
}

TEST_F(PudMhcsTest, ACandidateThatStepsOntoARefutedNodeReleasesItsWitness) {
    tree.branch(0, {1, 2});
    tree.branch(1, {5});
    tree.refute(1);
    witnessed_under(1, 10);
    ON_CALL(advance_witness, advance(10u))
        .WillByDefault(Return(pud_witness_advance{.search_root = pud_replay_at(5, 0), .next_sibling = 1}));
    EXPECT_CALL(remove_witness, remove(10u));
    EXPECT_FALSE(mhcs.try_add(pud_replay_at(0, 0)).has_value());
}

TEST_F(PudMhcsTest, AnExpandedSelfWitnessIsJustifiedAgainBelowIt) {
    tree.leaf(1);
    const pud_mhcs_head_id head = mhcs.try_add(pud_replay_at(1, 0)).value();
    tree.branch(1, {2, 3});
    witnessed_under(2, 10);
    witnessed_under(3, 11);
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(1)), IsEmpty());
    EXPECT_CALL(remove_witness, remove(10u));
    EXPECT_CALL(remove_witness, remove(11u));
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ARefutedSelfWitnessRefutesTheCandidate) {
    tree.leaf(1);
    const pud_mhcs_head_id head = mhcs.try_add(pud_replay_at(1, 0)).value();
    tree.refute(1);
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(1)), ElementsAre(head));
}

TEST_F(PudMhcsTest, InvalidatingALeafIsPassedOnToTheWitnessSearches) {
    EXPECT_CALL(invalidate_witnesses, invalidate_leaf(pud_test_node(3)));
    mhcs.invalidate_leaf(pud_test_node(3));
}

TEST_F(PudMhcsTest, AnExhaustedWitnessIsReplacedFromTheNextChild) {
    tree.branch(0, {1, 2, 3});
    witnessed_under(1, 10);
    witnessed_under(2, 11);
    witnessed_under(3, 12);
    const pud_mhcs_head_id head = mhcs.try_add(pud_replay_at(0, 0)).value();
    exhaust({10});
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(7)), IsEmpty());
    EXPECT_CALL(remove_witness, remove(11u));
    EXPECT_CALL(remove_witness, remove(12u));
    mhcs.remove(head);
}

TEST_F(PudMhcsTest, ACandidateLearnsOfEveryExhaustedWitnessBeforeItResumes) {
    tree.branch(0, {1, 2, 3, 4});
    witnessed_under(1, 10);
    witnessed_under(2, 11);
    witnessed_under(3, 12);
    witnessed_under(4, 13);
    mhcs.try_add(pud_replay_at(0, 0));
    exhaust({10, 11});
    EXPECT_CALL(add_witness, try_add(_)).Times(AnyNumber());
    EXPECT_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(3)))).Times(1);
    EXPECT_CALL(add_witness, try_add(Field(&pud_replay::node, pud_test_node(4)))).Times(1);
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(7)), IsEmpty());
}

TEST_F(PudMhcsTest, ExhaustingBothWitnessesWithNothingLeftRefutesTheCandidate) {
    tree.branch(0, {1, 2});
    witnessed_under(1, 10);
    witnessed_under(2, 11);
    const pud_mhcs_head_id head = mhcs.try_add(pud_replay_at(0, 0)).value();
    exhaust({10, 11});
    EXPECT_CALL(remove_witness, remove(_)).Times(0);
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(7)), ElementsAre(head));
}

TEST_F(PudMhcsTest, ExhaustedWitnessesReachOnlyTheirOwnCandidate) {
    tree.branch(0, {1, 2});
    tree.branch(20, {21, 22});
    witnessed_under(1, 10);
    witnessed_under(2, 11);
    witnessed_under(21, 30);
    witnessed_under(22, 31);
    const pud_mhcs_head_id first = mhcs.try_add(pud_replay_at(0, 0)).value();
    const pud_mhcs_head_id second = mhcs.try_add(pud_replay_at(20, 0)).value();
    exhaust({10, 11});
    EXPECT_THAT(mhcs.invalidate_leaf(pud_test_node(7)), ElementsAre(first));
    EXPECT_CALL(remove_witness, remove(30u));
    EXPECT_CALL(remove_witness, remove(31u));
    mhcs.remove(second);
}

TEST_F(PudMhcsTest, AForkReplaysThePathAndForksBothWitnessesOntoTheNewEnvironment) {
    const pud_mhcs_head_id head = add_stepped_choice_point();
    EXPECT_CALL(
        fork_witness,
        try_fork(10u, AllOf(Field(&pud_replay::node, pud_test_node(5)), Field(&pud_replay::frame_offset, 9u))))
        .WillOnce(Return(20u));
    EXPECT_CALL(
        fork_witness,
        try_fork(12u, AllOf(Field(&pud_replay::node, pud_test_node(6)), Field(&pud_replay::frame_offset, 9u))))
        .WillOnce(Return(21u));
    const pud_mhcs_head_id fork = mhcs.try_fork(head, pud_replay_at(0, 9)).value();
    EXPECT_THAT(mhcs.path(fork), ElementsAre(pud_test_node(0), pud_test_node(1)));
    EXPECT_CALL(remove_witness, remove(20u));
    EXPECT_CALL(remove_witness, remove(21u));
    mhcs.remove(fork);
}

TEST_F(PudMhcsTest, AForkWhosePathNoLongerReplaysIsNotAdmitted) {
    const pud_mhcs_head_id head = add_stepped_choice_point();
    ON_CALL(tree.descend, descend(Field(&pud_replay::frame_offset, 9u), pud_test_node(1)))
        .WillByDefault(Return(std::nullopt));
    EXPECT_CALL(fork_witness, try_fork(_, _)).Times(0);
    EXPECT_FALSE(mhcs.try_fork(head, pud_replay_at(0, 9)).has_value());
}

TEST_F(PudMhcsTest, AForkThatLosesAWitnessReplacesItFromTheRemainingChildren) {
    tree.branch(0, {1, 2, 3});
    witnessed_under(1, 10);
    witnessed_under(2, 11);
    const pud_mhcs_head_id head = mhcs.try_add(pud_replay_at(0, 0)).value();
    ON_CALL(fork_witness, try_fork(10u, _)).WillByDefault(Return(20u));
    ON_CALL(fork_witness, try_fork(11u, _)).WillByDefault(Return(std::nullopt));
    witnessed_under(3, 22);
    const pud_mhcs_head_id fork = mhcs.try_fork(head, pud_replay_at(0, 9)).value();
    EXPECT_CALL(remove_witness, remove(20u));
    EXPECT_CALL(remove_witness, remove(22u));
    mhcs.remove(fork);
}
