#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unit_query_recorder.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

struct MockTryAddQueryHead {
    MOCK_METHOD(pud_mhq_head_admission, try_add_head, (const std::vector<int>&));
};
struct MockTryForkQueryHead {
    MOCK_METHOD(pud_mhq_head_admission, try_fork_head, (pud_mhq_head_id, const std::vector<int>&));
};
struct MockCandidateSpecializationRefuted {
    MOCK_METHOD(std::optional<pud_forced_query_propagation>, candidate_specialization_refuted, (pud_mhcs_head_id));
};
struct MockPushUnitQuery {
    MOCK_METHOD(void, push, (pud_mhq_head_id));
};

using test_recorder_t = pud_unit_query_recorder<
    int, StrictMock<MockTryAddQueryHead>, StrictMock<MockTryForkQueryHead>,
    StrictMock<MockCandidateSpecializationRefuted>, StrictMock<MockPushUnitQuery>>;

pud_mhq_head_admission live(pud_mhq_head_id head_id) {
    return {head_id, std::nullopt};
}

pud_mhq_head_admission unit(pud_mhq_head_id head_id) {
    return {head_id, pud_unit_query{head_id}};
}

pud_mhq_head_admission refuted(pud_mhq_head_id head_id) {
    return {head_id, pud_refuted_query{head_id}};
}

struct PudUnitQueryRecorderTest : public ::testing::Test {
    StrictMock<MockTryAddQueryHead> adds;
    StrictMock<MockTryForkQueryHead> forks;
    StrictMock<MockCandidateSpecializationRefuted> refutations;
    StrictMock<MockPushUnitQuery> unit_queries;
    test_recorder_t recorder{adds, forks, refutations, unit_queries};
    std::vector<int> root_descents{1, 2};
};

}  // namespace

TEST_F(PudUnitQueryRecorderTest, UnitAdmissionIsPushedAndPassedThrough) {
    InSequence in_order;
    EXPECT_CALL(adds, try_add_head(root_descents)).WillOnce(Return(unit(7)));
    EXPECT_CALL(unit_queries, push(7u));
    EXPECT_EQ(recorder.try_add_head(root_descents), unit(7));
}

TEST_F(PudUnitQueryRecorderTest, LiveOrRefutedAdmissionsAreOnlyPassedThrough) {
    EXPECT_CALL(adds, try_add_head(_)).WillOnce(Return(live(7))).WillOnce(Return(refuted(8)));
    EXPECT_EQ(recorder.try_add_head(root_descents), live(7));
    EXPECT_EQ(recorder.try_add_head(root_descents), refuted(8));
}

TEST_F(PudUnitQueryRecorderTest, UnitForkIsPushedAndPassedThrough) {
    EXPECT_CALL(forks, try_fork_head(3u, root_descents)).WillOnce(Return(unit(9)));
    EXPECT_CALL(unit_queries, push(9u));
    EXPECT_EQ(recorder.try_fork_head(3, root_descents), unit(9));
}

TEST_F(PudUnitQueryRecorderTest, LiveForkIsOnlyPassedThrough) {
    EXPECT_CALL(forks, try_fork_head(3u, _)).WillOnce(Return(live(9)));
    EXPECT_EQ(recorder.try_fork_head(3, root_descents), live(9));
}

TEST_F(PudUnitQueryRecorderTest, RefutationLeavingOneCandidateIsPushed) {
    EXPECT_CALL(refutations, candidate_specialization_refuted(20u))
        .WillOnce(Return(pud_forced_query_propagation{pud_unit_query{4}}));
    EXPECT_CALL(unit_queries, push(4u));
    EXPECT_EQ(recorder.candidate_specialization_refuted(20), pud_forced_query_propagation{pud_unit_query{4}});
}

TEST_F(PudUnitQueryRecorderTest, RefutationThatRefutesOrChangesNothingIsOnlyPassedThrough) {
    EXPECT_CALL(refutations, candidate_specialization_refuted(_))
        .WillOnce(Return(pud_forced_query_propagation{pud_refuted_query{4}}))
        .WillOnce(Return(std::nullopt));
    EXPECT_EQ(recorder.candidate_specialization_refuted(20), pud_forced_query_propagation{pud_refuted_query{4}});
    EXPECT_EQ(recorder.candidate_specialization_refuted(21), std::nullopt);
}
