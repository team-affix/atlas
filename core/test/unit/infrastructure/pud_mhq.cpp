#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhq.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_query_resume_context.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct descent_t {
    pud_node_id node;
    bool operator==(const descent_t&) const = default;
};

struct HeadOps {
    MOCK_METHOD(pud_query_resume_context, resume, ());
    MOCK_METHOD((std::optional<pud_forced_unfold_reason>), candidate_refuted, (pud_mhcs_head_id));
};

struct MockHead {
    static HeadOps* ops;
    pud_query_resume_context resume() { return ops->resume(); }
    std::optional<pud_forced_unfold_reason> candidate_refuted(pud_mhcs_head_id candidate_id) {
        return ops->candidate_refuted(candidate_id);
    }
};

HeadOps* MockHead::ops = nullptr;

struct MockMakeHead {
    MOCK_METHOD(MockHead, make, (const std::vector<descent_t>&));
};

struct MockForkHead {
    MOCK_METHOD(MockHead, fork, (const MockHead&, const std::vector<descent_t>&));
};

using test_mhq_t = pud_mhq<descent_t, MockHead, MockMakeHead, MockForkHead>;

struct PudMhqTest : public ::testing::Test {
    HeadOps ops;
    NiceMock<MockMakeHead> make_head;
    NiceMock<MockForkHead> fork_head;
    test_mhq_t mhq{make_head, fork_head};
    descent_t axiom_a{1};
    descent_t axiom_b{2};
    std::vector<descent_t> two_axioms{axiom_a, axiom_b};
    pud_mhcs_head_id candidate_a = 10;
    pud_mhcs_head_id candidate_b = 20;
    pud_mhq_head_id parent_id = 0;
    pud_mhq_head_id child_id = 1;

    void SetUp() override {
        MockHead::ops = &ops;
        ON_CALL(make_head, make(_)).WillByDefault(Return(MockHead{}));
        ON_CALL(fork_head, fork(_, _)).WillByDefault(Return(MockHead{}));
    }

    void add_two_live() {
        EXPECT_CALL(ops, resume()).WillOnce(Return(pud_query_resume_context{
            .candidate_ids = {candidate_a, candidate_b},
            .forced_unfold_reason = std::nullopt,
        }));
        EXPECT_EQ(mhq.try_add_head(two_axioms), std::nullopt);
    }
};

TEST_F(PudMhqTest, AddWithTwoLiveCandidatesHasNoForcedPropagation) {
    EXPECT_CALL(make_head, make(two_axioms)).WillOnce(Return(MockHead{}));
    add_two_live();
}

TEST_F(PudMhqTest, AddThatIsImmediatelyUnitIsRemembered) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(pud_query_resume_context{
        .candidate_ids = {candidate_a},
        .forced_unfold_reason = pud_forced_unfold_reason::unit,
    }));
    EXPECT_EQ(
        mhq.try_add_head({axiom_a}),
        std::optional<pud_forced_query_propagation>{pud_unit_query{.head_id = parent_id}});

    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::refuted));
    EXPECT_EQ(
        mhq.candidate_specialization_refuted(candidate_a),
        std::optional<pud_forced_query_propagation>{pud_refuted_query{.head_id = parent_id}});
}

TEST_F(PudMhqTest, AddThatIsImmediatelyRefutedIsForgotten) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(pud_query_resume_context{
        .candidate_ids = {},
        .forced_unfold_reason = pud_forced_unfold_reason::refuted,
    }));
    EXPECT_EQ(
        mhq.try_add_head(two_axioms),
        std::optional<pud_forced_query_propagation>{pud_refuted_query{.head_id = parent_id}});
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), std::nullopt);
}

TEST_F(PudMhqTest, UnknownCandidateRefutationIsIgnored) {
    EXPECT_EQ(mhq.candidate_specialization_refuted(99), std::nullopt);
}

TEST_F(PudMhqTest, RefutingOneOfTwoLiveCandidatesIsUnit) {
    add_two_live();
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(
        mhq.candidate_specialization_refuted(candidate_a),
        std::optional<pud_forced_query_propagation>{pud_unit_query{.head_id = parent_id}});
}

TEST_F(PudMhqTest, RefutingLastLiveCandidateIsRefuted) {
    add_two_live();
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(
        mhq.candidate_specialization_refuted(candidate_a),
        std::optional<pud_forced_query_propagation>{pud_unit_query{.head_id = parent_id}});

    EXPECT_CALL(ops, candidate_refuted(candidate_b)).WillOnce(Return(pud_forced_unfold_reason::refuted));
    EXPECT_EQ(
        mhq.candidate_specialization_refuted(candidate_b),
        std::optional<pud_forced_query_propagation>{pud_refuted_query{.head_id = parent_id}});
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_b), std::nullopt);
}

TEST_F(PudMhqTest, RemoveUnlinksCandidates) {
    add_two_live();
    mhq.remove_head(parent_id);
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_b), std::nullopt);
}

TEST_F(PudMhqTest, RemoveOfUnknownIdIsNoop) {
    add_two_live();
    mhq.remove_head(parent_id + 100);
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(
        mhq.candidate_specialization_refuted(candidate_a),
        std::optional<pud_forced_query_propagation>{pud_unit_query{.head_id = parent_id}});
}

TEST_F(PudMhqTest, ForkWithTwoLiveCandidatesHasNoForcedPropagation) {
    add_two_live();
    EXPECT_CALL(fork_head, fork(_, two_axioms)).WillOnce(Return(MockHead{}));
    EXPECT_CALL(ops, resume()).WillOnce(Return(pud_query_resume_context{
        .candidate_ids = {110, 120},
        .forced_unfold_reason = std::nullopt,
    }));
    EXPECT_EQ(mhq.try_fork_head(parent_id, two_axioms), std::nullopt);
}

TEST_F(PudMhqTest, ForkThatIsImmediatelyUnitIsRemembered) {
    add_two_live();
    EXPECT_CALL(ops, resume()).WillOnce(Return(pud_query_resume_context{
        .candidate_ids = {110},
        .forced_unfold_reason = pud_forced_unfold_reason::unit,
    }));
    EXPECT_EQ(
        mhq.try_fork_head(parent_id, two_axioms),
        std::optional<pud_forced_query_propagation>{pud_unit_query{.head_id = child_id}});

    EXPECT_CALL(ops, candidate_refuted(110)).WillOnce(Return(pud_forced_unfold_reason::refuted));
    EXPECT_EQ(
        mhq.candidate_specialization_refuted(110),
        std::optional<pud_forced_query_propagation>{pud_refuted_query{.head_id = child_id}});
}

TEST_F(PudMhqTest, ForkThatIsImmediatelyRefutedIsForgotten) {
    add_two_live();
    EXPECT_CALL(ops, resume()).WillOnce(Return(pud_query_resume_context{
        .candidate_ids = {},
        .forced_unfold_reason = pud_forced_unfold_reason::refuted,
    }));
    EXPECT_EQ(
        mhq.try_fork_head(parent_id, two_axioms),
        std::optional<pud_forced_query_propagation>{pud_refuted_query{.head_id = child_id}});
    EXPECT_EQ(mhq.candidate_specialization_refuted(110), std::nullopt);
}

}  // namespace
