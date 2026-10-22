#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_leaf_invalidator.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {

struct MockInvalidateWitnessLeaf {
    MOCK_METHOD(std::vector<pud_mhws_head_id>, invalidate_leaf, (pud_node_id));
};

struct MockInvalidateCandidateLeaf {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, invalidate_leaf, (pud_node_id));
};

struct MockWitnessesRefuted {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, witnesses_refuted, (const std::vector<pud_mhws_head_id>&));
};

struct MockCandidateSpecializationRefuted {
    MOCK_METHOD(std::optional<pud_forced_query_propagation>, candidate_specialization_refuted, (pud_mhcs_head_id));
};

using test_invalidator_t = pud_leaf_invalidator<
    NiceMock<MockInvalidateWitnessLeaf>,
    NiceMock<MockInvalidateCandidateLeaf>,
    NiceMock<MockWitnessesRefuted>,
    NiceMock<MockCandidateSpecializationRefuted>>;

struct PudLeafInvalidatorTest : public ::testing::Test {
    NiceMock<MockInvalidateWitnessLeaf> witness_leaves;
    NiceMock<MockInvalidateCandidateLeaf> candidate_leaves;
    NiceMock<MockWitnessesRefuted> witnesses_refuted;
    NiceMock<MockCandidateSpecializationRefuted> candidates_refuted;
    test_invalidator_t invalidator{witness_leaves, candidate_leaves, witnesses_refuted, candidates_refuted};
    pud_node_id leaf_a = 1;
    pud_node_id leaf_b = 2;
};

TEST_F(PudLeafInvalidatorTest, EveryLeafIsInvalidatedInBothLayers) {
    EXPECT_CALL(witness_leaves, invalidate_leaf(leaf_a));
    EXPECT_CALL(witness_leaves, invalidate_leaf(leaf_b));
    EXPECT_CALL(candidate_leaves, invalidate_leaf(leaf_a));
    EXPECT_CALL(candidate_leaves, invalidate_leaf(leaf_b));
    invalidator.invalidate({leaf_a, leaf_b});
}

TEST_F(PudLeafInvalidatorTest, ExhaustedWitnessesOfEveryLeafAreReportedInOneBatch) {
    EXPECT_CALL(witness_leaves, invalidate_leaf(leaf_a)).WillOnce(Return(std::vector<pud_mhws_head_id>{3}));
    EXPECT_CALL(witness_leaves, invalidate_leaf(leaf_b)).WillOnce(Return(std::vector<pud_mhws_head_id>{4, 5}));
    EXPECT_CALL(witnesses_refuted, witnesses_refuted(ElementsAre(3u, 4u, 5u)));
    invalidator.invalidate({leaf_a, leaf_b});
}

TEST_F(PudLeafInvalidatorTest, EveryWitnessSearchMovesBeforeAnyCandidateIsTold) {
    InSequence in_order;
    EXPECT_CALL(witness_leaves, invalidate_leaf(leaf_a));
    EXPECT_CALL(witness_leaves, invalidate_leaf(leaf_b));
    EXPECT_CALL(candidate_leaves, invalidate_leaf(leaf_a));
    EXPECT_CALL(candidate_leaves, invalidate_leaf(leaf_b));
    EXPECT_CALL(witnesses_refuted, witnesses_refuted(_));
    invalidator.invalidate({leaf_a, leaf_b});
}

TEST_F(PudLeafInvalidatorTest, CandidatesRefutedEitherWayAreReportedToTheQueries) {
    EXPECT_CALL(candidate_leaves, invalidate_leaf(leaf_a)).WillOnce(Return(std::vector<pud_mhcs_head_id>{7}));
    EXPECT_CALL(witnesses_refuted, witnesses_refuted(_)).WillOnce(Return(std::vector<pud_mhcs_head_id>{8}));
    EXPECT_CALL(candidates_refuted, candidate_specialization_refuted(7u));
    EXPECT_CALL(candidates_refuted, candidate_specialization_refuted(8u));
    invalidator.invalidate({leaf_a});
}

TEST_F(PudLeafInvalidatorTest, ForcedQueriesAreReturned) {
    EXPECT_CALL(candidate_leaves, invalidate_leaf(leaf_a)).WillOnce(Return(std::vector<pud_mhcs_head_id>{7, 8}));
    EXPECT_CALL(candidates_refuted, candidate_specialization_refuted(7u))
        .WillOnce(Return(pud_forced_query_propagation{pud_unit_query{.head_id = 20}}));
    EXPECT_CALL(candidates_refuted, candidate_specialization_refuted(8u))
        .WillOnce(Return(pud_forced_query_propagation{pud_refuted_query{.head_id = 21}}));
    EXPECT_THAT(
        invalidator.invalidate({leaf_a}),
        UnorderedElementsAre(
            pud_forced_query_propagation{pud_unit_query{.head_id = 20}},
            pud_forced_query_propagation{pud_refuted_query{.head_id = 21}}));
}

TEST_F(PudLeafInvalidatorTest, CandidatesThatForceNothingAreDropped) {
    EXPECT_CALL(candidate_leaves, invalidate_leaf(leaf_a)).WillOnce(Return(std::vector<pud_mhcs_head_id>{7}));
    EXPECT_CALL(candidates_refuted, candidate_specialization_refuted(7u)).WillOnce(Return(std::nullopt));
    EXPECT_THAT(invalidator.invalidate({leaf_a}), IsEmpty());
}

TEST_F(PudLeafInvalidatorTest, NoLeavesForceNothing) {
    EXPECT_CALL(candidates_refuted, candidate_specialization_refuted(_)).Times(0);
    EXPECT_THAT(invalidator.invalidate({}), IsEmpty());
}

}  // namespace
