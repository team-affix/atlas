#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhws.hpp"
#include "pud_search_tree.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::NiceMock;

namespace {

using test_mhws_t = pud_mhws<
    NiceMock<MockDescendReplay>, NiceMock<MockCheckNodeLeaf>, NiceMock<MockCheckNodeRefuted>, NiceMock<MockGetChildren>>;

struct PudMhwsTest : public ::testing::Test {
    pud_search_tree tree;
    test_mhws_t mhws{tree.descend, tree.leaves, tree.refuted, tree.children};

    // Refutes the leaf and reports which searches it exhausts; a search standing elsewhere is untouched.
    std::vector<pud_mhws_head_id> refute_and_invalidate(pud_node_id leaf) {
        tree.refute(leaf);
        return mhws.invalidate_leaf(pud_test_node(leaf));
    }
};

}

TEST_F(PudMhwsTest, ALeafSearchRootIsItsOwnWitness) {
    tree.leaf(1);
    const std::optional<pud_mhws_head_id> head = mhws.try_add(pud_replay_at(1, 0));
    ASSERT_TRUE(head.has_value());
    EXPECT_THAT(refute_and_invalidate(1), ElementsAre(head.value()));
}

TEST_F(PudMhwsTest, ASearchThatReachesNoLeafIsNotAdmitted) {
    tree.branch(0, {1, 2});
    tree.branch(1, {});
    tree.leaf(2);
    tree.block(2);
    EXPECT_FALSE(mhws.try_add(pud_replay_at(0, 0)).has_value());
}

TEST_F(PudMhwsTest, ARefutedSearchRootIsNotAdmitted) {
    tree.leaf(1);
    tree.refute(1);
    EXPECT_FALSE(mhws.try_add(pud_replay_at(1, 0)).has_value());
}

TEST_F(PudMhwsTest, TheWitnessIsTheFirstLeafDepthFirstInChildOrder) {
    tree.branch(0, {1, 2});
    tree.branch(1, {3});
    tree.leaf(3);
    tree.leaf(2);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(0, 0)).value();
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(2)), IsEmpty());
    EXPECT_THAT(refute_and_invalidate(3), IsEmpty());
    EXPECT_THAT(refute_and_invalidate(2), ElementsAre(head));
}

TEST_F(PudMhwsTest, ChildrenAreSteppedIntoInTheirParentsModule) {
    tree.branch(0, {1});
    tree.leaf(1);
    EXPECT_CALL(
        tree.descend,
        descend(Field(&pud_replay::node, pud_test_node(0)), pud_global_node_id{.module = k_test_module, .node = 1}));
    mhws.try_add(pud_replay_at(0, 0));
}

TEST_F(PudMhwsTest, BlockedAndRefutedChildrenAreSkipped) {
    tree.branch(0, {1, 2, 3});
    tree.leaf(1);
    tree.block(1);
    tree.leaf(2);
    tree.refute(2);
    tree.leaf(3);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(0, 0)).value();
    EXPECT_THAT(refute_and_invalidate(3), ElementsAre(head));
}

TEST_F(PudMhwsTest, ARefutedInteriorNodeHidesItsLeaves) {
    tree.branch(0, {1, 2});
    tree.branch(1, {3});
    tree.leaf(3);
    tree.refute(1);
    tree.leaf(2);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(0, 0)).value();
    EXPECT_THAT(refute_and_invalidate(2), ElementsAre(head));
}

TEST_F(PudMhwsTest, AnExpandedWitnessIsSearchedBelow) {
    tree.branch(0, {1});
    tree.leaf(1);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(0, 0)).value();
    tree.branch(1, {4});
    tree.leaf(4);
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(1)), IsEmpty());
    EXPECT_THAT(refute_and_invalidate(4), ElementsAre(head));
}

TEST_F(PudMhwsTest, ADeadSubtreeIsLeftForTheParentsNextChild) {
    tree.branch(0, {1, 2});
    tree.branch(1, {3, 4});
    tree.leaf(3);
    tree.leaf(4);
    tree.leaf(2);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(0, 0)).value();
    tree.refute(4);
    EXPECT_THAT(refute_and_invalidate(3), IsEmpty());
    EXPECT_THAT(refute_and_invalidate(2), ElementsAre(head));
}

TEST_F(PudMhwsTest, AnExhaustedSearchIsReportedOnceAndDropped) {
    tree.leaf(1);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(1, 0)).value();
    EXPECT_THAT(refute_and_invalidate(1), ElementsAre(head));
    EXPECT_THAT(mhws.invalidate_leaf(pud_test_node(1)), IsEmpty());
}

TEST_F(PudMhwsTest, InvalidatingAnUnwitnessedLeafChangesNothing) {
    tree.leaf(1);
    tree.leaf(2);
    mhws.try_add(pud_replay_at(1, 0));
    EXPECT_THAT(refute_and_invalidate(2), IsEmpty());
}

TEST_F(PudMhwsTest, EverySearchOnAnInvalidatedLeafResumes) {
    tree.branch(0, {1, 2});
    tree.leaf(1);
    tree.leaf(2);
    const pud_mhws_head_id first = mhws.try_add(pud_replay_at(0, 0)).value();
    const pud_mhws_head_id second = mhws.try_add(pud_replay_at(1, 0)).value();
    EXPECT_THAT(refute_and_invalidate(1), ElementsAre(second));
    EXPECT_THAT(refute_and_invalidate(2), ElementsAre(first));
}

TEST_F(PudMhwsTest, ARemovedSearchIsNeverResumed) {
    tree.leaf(1);
    mhws.remove(mhws.try_add(pud_replay_at(1, 0)).value());
    EXPECT_THAT(refute_and_invalidate(1), IsEmpty());
}

TEST_F(PudMhwsTest, AdvanceStepsTheSearchRootOntoThePathAndReportsTheNextSibling) {
    tree.branch(0, {1, 2});
    tree.branch(1, {3});
    tree.leaf(3);
    tree.leaf(2);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(0, 0)).value();
    EXPECT_EQ(mhws.advance(head), (pud_witness_advance{.search_root = pud_replay_at(1, 0), .next_sibling = 1}));
    EXPECT_EQ(mhws.advance(head), (pud_witness_advance{.search_root = pud_replay_at(3, 0), .next_sibling = 1}));
}

TEST_F(PudMhwsTest, AdvanceKeepsTheSearchResumableBelowItsNewRoot) {
    tree.branch(0, {1});
    tree.branch(1, {3, 4});
    tree.leaf(3);
    tree.leaf(4);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(0, 0)).value();
    mhws.advance(head);
    EXPECT_THAT(refute_and_invalidate(3), IsEmpty());
    EXPECT_THAT(refute_and_invalidate(4), ElementsAre(head));
}

TEST_F(PudMhwsTest, AdvancingASearchStandingOnItsRootDropsIt) {
    tree.leaf(1);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(1, 0)).value();
    EXPECT_FALSE(mhws.advance(head).has_value());
    EXPECT_THAT(refute_and_invalidate(1), IsEmpty());
}

TEST_F(PudMhwsTest, AForkReplaysTheSearchPathFromItsNewRoot) {
    tree.branch(0, {1, 2});
    tree.branch(1, {3});
    tree.leaf(3);
    tree.leaf(2);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(0, 0)).value();
    EXPECT_CALL(tree.descend, descend(_, _)).Times(AnyNumber());
    EXPECT_CALL(tree.descend, descend(Field(&pud_replay::frame_offset, 9u), pud_test_node(1)));
    EXPECT_CALL(tree.descend, descend(Field(&pud_replay::frame_offset, 9u), pud_test_node(3)));
    const pud_mhws_head_id fork = mhws.try_fork(head, pud_replay_at(0, 9)).value();
    EXPECT_THAT(refute_and_invalidate(3), IsEmpty());
    EXPECT_THAT(refute_and_invalidate(2), ::testing::UnorderedElementsAre(head, fork));
}

TEST_F(PudMhwsTest, AForkThatCannotReplayItsPathMovesOnToLaterSiblings) {
    tree.branch(0, {1, 2});
    tree.branch(1, {3});
    tree.leaf(3);
    tree.leaf(2);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(0, 0)).value();
    ON_CALL(tree.descend, descend(Field(&pud_replay::frame_offset, 9u), pud_test_node(1)))
        .WillByDefault(::testing::Return(std::nullopt));
    const pud_mhws_head_id fork = mhws.try_fork(head, pud_replay_at(0, 9)).value();
    EXPECT_THAT(refute_and_invalidate(2), ElementsAre(fork));
    EXPECT_THAT(refute_and_invalidate(3), ElementsAre(head));
}

TEST_F(PudMhwsTest, AForkThatReachesNoLeafIsNotAdmitted) {
    tree.branch(0, {1});
    tree.leaf(1);
    const pud_mhws_head_id head = mhws.try_add(pud_replay_at(0, 0)).value();
    ON_CALL(tree.descend, descend(Field(&pud_replay::frame_offset, 9u), _)).WillByDefault(::testing::Return(std::nullopt));
    EXPECT_FALSE(mhws.try_fork(head, pud_replay_at(0, 9)).has_value());
}
