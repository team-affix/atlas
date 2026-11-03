#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_witness_search_recorder.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockMhws {
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_add, (pud_replay));
    MOCK_METHOD(std::optional<pud_mhws_head_id>, try_fork, (pud_mhws_head_id, pud_replay));
    MOCK_METHOD(std::optional<pud_witness_advance>, advance, (pud_mhws_head_id));
    MOCK_METHOD(void, remove, (pud_mhws_head_id));
    MOCK_METHOD(std::vector<pud_mhws_head_id>, invalidate_leaf, (pud_global_node_id));
    MOCK_METHOD(std::optional<pud_global_node_id>, find_witness, (pud_mhws_head_id), (const));
};

pud_replay replay_at(pud_node_id node) {
    return pud_replay{.node = pud_test_node(node), .frame_offset = 0, .ubs_state = {}};
}

struct PudWitnessSearchRecorderTest : public ::testing::Test {
    NiceMock<MockMhws> searches;
    pud_witness_search_recorder<NiceMock<MockMhws>> log{searches};
};

}

TEST_F(PudWitnessSearchRecorderTest, AddsAndForksAreForwarded) {
    EXPECT_CALL(searches, try_add(replay_at(5))).WillOnce(Return(40));
    EXPECT_CALL(searches, try_fork(40, replay_at(6))).WillOnce(Return(std::nullopt));
    EXPECT_EQ(log.try_add(replay_at(5)), 40u);
    EXPECT_EQ(log.try_fork(40, replay_at(6)), std::nullopt);
}

TEST_F(PudWitnessSearchRecorderTest, AdvanceRemoveAndInvalidateAreForwarded) {
    const pud_witness_advance advanced{.search_root = replay_at(7), .next_sibling = 1};
    EXPECT_CALL(searches, advance(40)).WillOnce(Return(advanced));
    EXPECT_CALL(searches, remove(41));
    EXPECT_CALL(searches, invalidate_leaf(pud_test_node(3))).WillOnce(Return(std::vector<pud_mhws_head_id>{42}));
    EXPECT_EQ(log.advance(40), advanced);
    log.remove(41);
    EXPECT_THAT(log.invalidate_leaf(pud_test_node(3)), ElementsAre(42));
}

TEST_F(PudWitnessSearchRecorderTest, ASearchsNodeIsTheWitnessTheSearchRestsOn) {
    EXPECT_CALL(searches, find_witness(40)).WillOnce(Return(pud_test_node(9)));
    EXPECT_EQ(log.node_of(40), pud_test_node(9));
}

TEST_F(PudWitnessSearchRecorderTest, AddsAndForksAreRecordedOnlyBetweenBeginAndTake) {
    EXPECT_CALL(searches, try_add(_)).WillOnce(Return(40)).WillOnce(Return(41)).WillOnce(Return(42));
    EXPECT_CALL(searches, try_fork(_, _)).WillOnce(Return(43)).WillOnce(Return(std::nullopt));
    log.try_add(replay_at(5));
    log.begin_recording_adds();
    log.try_add(replay_at(6));
    log.try_fork(41, replay_at(6));
    log.try_fork(41, replay_at(7));
    EXPECT_THAT(log.take_recorded_adds(), ElementsAre(41, 43));
    log.try_add(replay_at(7));
    log.begin_recording_adds();
    EXPECT_THAT(log.take_recorded_adds(), IsEmpty());
}

TEST_F(PudWitnessSearchRecorderTest, NothingIsUnsettledAtFirst) {
    EXPECT_THAT(log.take_unsettled_leaves(), IsEmpty());
    EXPECT_THAT(log.take_unsettled_searches(), IsEmpty());
}

TEST_F(PudWitnessSearchRecorderTest, AnInvalidatedLeafIsUnsettled) {
    log.invalidate_leaf(pud_test_node(3));
    log.invalidate_leaf(pud_test_node(4));
    EXPECT_THAT(log.take_unsettled_leaves(), ElementsAre(pud_test_node(3), pud_test_node(4)));
    EXPECT_THAT(log.take_unsettled_searches(), IsEmpty());
}

TEST_F(PudWitnessSearchRecorderTest, AnAdvancedOrRemovedSearchIsUnsettled) {
    log.advance(40);
    log.remove(41);
    EXPECT_THAT(log.take_unsettled_searches(), ElementsAre(40, 41));
    EXPECT_THAT(log.take_unsettled_leaves(), IsEmpty());
}

TEST_F(PudWitnessSearchRecorderTest, TakingTheUnsettledLeavesAndSearchesEmptiesTheLogs) {
    log.invalidate_leaf(pud_test_node(3));
    log.remove(41);
    log.take_unsettled_leaves();
    log.take_unsettled_searches();
    EXPECT_THAT(log.take_unsettled_leaves(), IsEmpty());
    EXPECT_THAT(log.take_unsettled_searches(), IsEmpty());
}
