#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_candidate_witness_index.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockMhcs {
    MOCK_METHOD(std::optional<pud_mhcs_head_id>, try_add, (pud_replay));
    MOCK_METHOD(std::optional<pud_mhcs_head_id>, try_fork, (pud_mhcs_head_id, pud_replay));
    MOCK_METHOD(void, remove, (pud_mhcs_head_id));
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, invalidate_leaf, (pud_global_node_id));
    MOCK_METHOD(const std::vector<pud_global_node_id>&, path, (pud_mhcs_head_id), (const));
};
struct MockWitnessLog {
    MOCK_METHOD(void, begin_recording_adds, ());
    MOCK_METHOD(std::vector<pud_mhws_head_id>, take_recorded_adds, ());
};

using test_index_t = pud_candidate_witness_index<NiceMock<MockMhcs>, NiceMock<MockWitnessLog>>;

pud_replay replay_at(pud_node_id node) {
    return pud_replay{.node = pud_test_node(node), .frame_offset = 0, .ubs_state = {}};
}

struct PudCandidateWitnessIndexTest : public ::testing::Test {
    NiceMock<MockMhcs> candidates;
    NiceMock<MockWitnessLog> witnesses;
    test_index_t index{candidates, witnesses};
    std::vector<pud_global_node_id> walked{pud_test_node(1), pud_test_node(2)};

    void SetUp() override {
        ON_CALL(candidates, path(_)).WillByDefault(ReturnRef(walked));
        ON_CALL(witnesses, take_recorded_adds()).WillByDefault(Return(std::vector<pud_mhws_head_id>{}));
    }
};

}

TEST_F(PudCandidateWitnessIndexTest, TheWitnessSearchesAnAddAddsAreRecordedAroundIt) {
    InSequence order;
    EXPECT_CALL(witnesses, begin_recording_adds());
    EXPECT_CALL(candidates, try_add(replay_at(4))).WillOnce(Return(30));
    EXPECT_CALL(witnesses, take_recorded_adds()).WillOnce(Return(std::vector<pud_mhws_head_id>{50, 51}));
    EXPECT_EQ(index.try_add(replay_at(4)), 30u);
    EXPECT_EQ(*index.find(30), (pud_candidate_witnesses{
        .self_witness = false, .self_leaf = {}, .witnesses = {50, 51}}));
}

TEST_F(PudCandidateWitnessIndexTest, ACandidateThatAddedNoSearchIsItsOwnWitnessAtTheEndOfItsPath) {
    ON_CALL(candidates, try_add(_)).WillByDefault(Return(30));
    index.try_add(replay_at(4));
    EXPECT_EQ(*index.find(30), (pud_candidate_witnesses{
        .self_witness = true, .self_leaf = pud_test_node(2), .witnesses = {}}));
}

TEST_F(PudCandidateWitnessIndexTest, AForkIsIndexedLikeAnAdd) {
    InSequence order;
    EXPECT_CALL(witnesses, begin_recording_adds());
    EXPECT_CALL(candidates, try_fork(30, replay_at(4))).WillOnce(Return(31));
    EXPECT_CALL(witnesses, take_recorded_adds()).WillOnce(Return(std::vector<pud_mhws_head_id>{52}));
    EXPECT_EQ(index.try_fork(30, replay_at(4)), 31u);
    EXPECT_THAT(index.find(31)->witnesses, ElementsAre(52));
}

TEST_F(PudCandidateWitnessIndexTest, AFailedAddStillEndsTheRecordingAndIndexesNothing) {
    InSequence order;
    EXPECT_CALL(witnesses, begin_recording_adds());
    EXPECT_CALL(candidates, try_add(_)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(witnesses, take_recorded_adds());
    EXPECT_EQ(index.try_add(replay_at(4)), std::nullopt);
    EXPECT_EQ(index.find(30), nullptr);
}

TEST_F(PudCandidateWitnessIndexTest, ARemovedCandidateLeavesTheIndexAndIsRemovedBelow) {
    ON_CALL(candidates, try_add(_)).WillByDefault(Return(30));
    index.try_add(replay_at(4));
    EXPECT_CALL(candidates, remove(30));
    index.remove(30);
    EXPECT_EQ(index.find(30), nullptr);
}

TEST_F(PudCandidateWitnessIndexTest, CandidatesAnInvalidatedLeafRefutesLeaveTheIndex) {
    EXPECT_CALL(candidates, try_add(_)).WillOnce(Return(30)).WillOnce(Return(31));
    index.try_add(replay_at(4));
    index.try_add(replay_at(5));
    EXPECT_CALL(candidates, invalidate_leaf(pud_test_node(2))).WillOnce(Return(std::vector<pud_mhcs_head_id>{30}));
    EXPECT_THAT(index.invalidate_leaf(pud_test_node(2)), ElementsAre(30));
    EXPECT_EQ(index.find(30), nullptr);
    EXPECT_NE(index.find(31), nullptr);
}
