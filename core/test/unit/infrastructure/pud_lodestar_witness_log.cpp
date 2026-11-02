#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_witness_log.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockMhws {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_add, (pud_replay));
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_fork, (pud_mhws_head_id, pud_replay));
    MOCK_METHOD(std::optional<pud_witness_advance>, advance, (pud_mhws_head_id));
    MOCK_METHOD(void, remove, (pud_mhws_head_id));
    MOCK_METHOD(std::vector<pud_mhws_head_id>, invalidate_leaf, (pud_global_node_id));
};
struct MockModules {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
    MOCK_METHOD(bool, is_leaf, (pud_global_node_id), (const));
    MOCK_METHOD(const std::vector<pud_node_id>&, get_children, (pud_global_node_id), (const));
};

using test_witness_log_t = pud_lodestar_witness_log<NiceMock<MockMhws>, NiceMock<MockModules>>;

pud_replay replay_at(pud_node_id node) {
    return pud_replay{.node = pud_test_node(node), .frame_offset = 0, .ubs_state = {}};
}

// Node 1 is interior with children 2 (refuted) and 3 (a leaf); every other node is a leaf.
struct PudLodestarWitnessLogTest : public ::testing::Test {
    NiceMock<MockMhws> searches;
    NiceMock<MockModules> modules;
    test_witness_log_t log{searches, modules};
    std::vector<pud_node_id> children_of_one{2, 3};

    void SetUp() override {
        ON_CALL(modules, is_refuted(_)).WillByDefault(Return(false));
        ON_CALL(modules, is_refuted(pud_test_node(2))).WillByDefault(Return(true));
        ON_CALL(modules, is_leaf(_)).WillByDefault(Return(true));
        ON_CALL(modules, is_leaf(pud_test_node(1))).WillByDefault(Return(false));
        ON_CALL(modules, get_children(pud_test_node(1))).WillByDefault(ReturnRef(children_of_one));
    }
};

}

TEST_F(PudLodestarWitnessLogTest, AnAddedSearchIsAtItsRootLeaf) {
    ON_CALL(searches, try_add(replay_at(5))).WillByDefault(Return(40));
    EXPECT_EQ(log.try_add(replay_at(5)), 40u);
    EXPECT_EQ(log.node_of(40), pud_test_node(5));
}

TEST_F(PudLodestarWitnessLogTest, ARootThatWasUnfoldedSettlesOnItsFirstUnrefutedDescendantLeaf) {
    ON_CALL(searches, try_add(replay_at(1))).WillByDefault(Return(40));
    log.try_add(replay_at(1));
    EXPECT_EQ(log.node_of(40), pud_test_node(3));
}

TEST_F(PudLodestarWitnessLogTest, AFailedAddOrForkIsNotRemembered) {
    ON_CALL(searches, try_add(_)).WillByDefault(Return(std::nullopt));
    ON_CALL(searches, try_fork(_, _)).WillByDefault(Return(std::nullopt));
    EXPECT_EQ(log.try_add(replay_at(5)), std::nullopt);
    EXPECT_EQ(log.try_fork(40, replay_at(6)), std::nullopt);
    EXPECT_EQ(log.node_of(40), std::nullopt);
}

TEST_F(PudLodestarWitnessLogTest, AForkIsAtItsOwnRootLeaf) {
    ON_CALL(searches, try_fork(40, replay_at(6))).WillByDefault(Return(41));
    EXPECT_EQ(log.try_fork(40, replay_at(6)), 41u);
    EXPECT_EQ(log.node_of(41), pud_test_node(6));
}

TEST_F(PudLodestarWitnessLogTest, AnAdvanceMovesTheSearchAndAFailedAdvanceForgetsIt) {
    ON_CALL(searches, try_add(_)).WillByDefault(Return(40));
    log.try_add(replay_at(5));
    EXPECT_CALL(searches, advance(40))
        .WillOnce(Return(pud_witness_advance{.search_root = replay_at(7), .next_sibling = 0}))
        .WillOnce(Return(std::nullopt));
    log.advance(40);
    EXPECT_EQ(log.node_of(40), pud_test_node(7));
    EXPECT_EQ(log.advance(40), std::nullopt);
    EXPECT_EQ(log.node_of(40), std::nullopt);
}

TEST_F(PudLodestarWitnessLogTest, ARemovedSearchIsForgottenAndRemovedBelow) {
    ON_CALL(searches, try_add(_)).WillByDefault(Return(40));
    log.try_add(replay_at(5));
    EXPECT_CALL(searches, remove(40));
    log.remove(40);
    EXPECT_EQ(log.node_of(40), std::nullopt);
}

TEST_F(PudLodestarWitnessLogTest, InvalidatingALeafForgetsExhaustedSearchesAndResettlesTheRest) {
    EXPECT_CALL(searches, try_add(_)).WillOnce(Return(40)).WillOnce(Return(41));
    log.try_add(replay_at(1));
    log.try_add(replay_at(5));
    ON_CALL(modules, is_leaf(pud_test_node(5))).WillByDefault(Return(false));
    std::vector<pud_node_id> children_of_five{8};
    ON_CALL(modules, get_children(pud_test_node(5))).WillByDefault(ReturnRef(children_of_five));
    EXPECT_CALL(searches, invalidate_leaf(pud_test_node(3))).WillOnce(Return(std::vector<pud_mhws_head_id>{40}));
    EXPECT_THAT(log.invalidate_leaf(pud_test_node(3)), ElementsAre(40));
    EXPECT_EQ(log.node_of(40), std::nullopt);
    EXPECT_CALL(searches, invalidate_leaf(pud_test_node(5))).WillOnce(Return(std::vector<pud_mhws_head_id>{}));
    log.invalidate_leaf(pud_test_node(5));
    EXPECT_EQ(log.node_of(41), pud_test_node(8));
}

TEST_F(PudLodestarWitnessLogTest, AddsAreRecordedOnlyBetweenBeginAndTake) {
    EXPECT_CALL(searches, try_add(_)).WillOnce(Return(40)).WillOnce(Return(41)).WillOnce(Return(42));
    log.try_add(replay_at(5));
    log.begin_recording_adds();
    log.try_add(replay_at(6));
    EXPECT_THAT(log.take_recorded_adds(), ElementsAre(41));
    log.try_add(replay_at(7));
    log.begin_recording_adds();
    EXPECT_THAT(log.take_recorded_adds(), IsEmpty());
}
