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

using propagation_t = std::optional<pud_forced_query_propagation>;

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
    pud_mhcs_head_id candidate_c = 30;
    pud_mhcs_head_id candidate_d = 40;
    pud_mhq_head_id first_id = 0;
    pud_mhq_head_id second_id = 1;
    pud_mhq_head_id third_id = 2;

    void SetUp() override {
        MockHead::ops = &ops;
        ON_CALL(make_head, make(_)).WillByDefault(Return(MockHead{}));
        ON_CALL(fork_head, fork(_, _)).WillByDefault(Return(MockHead{}));
    }

    static pud_query_resume_context live(const std::vector<pud_mhcs_head_id>& candidate_ids) {
        return {.candidate_ids = candidate_ids, .forced_unfold_reason = std::nullopt};
    }

    static pud_query_resume_context unit(pud_mhcs_head_id candidate_id) {
        return {.candidate_ids = {candidate_id}, .forced_unfold_reason = pud_forced_unfold_reason::unit};
    }

    static pud_query_resume_context refuted() {
        return {.candidate_ids = {}, .forced_unfold_reason = pud_forced_unfold_reason::refuted};
    }

    static propagation_t unit_query(pud_mhq_head_id head_id) {
        return pud_unit_query{.head_id = head_id};
    }

    static propagation_t refuted_query(pud_mhq_head_id head_id) {
        return pud_refuted_query{.head_id = head_id};
    }

    void add_two_live() {
        EXPECT_CALL(ops, resume()).WillOnce(Return(live({candidate_a, candidate_b})));
        EXPECT_EQ(mhq.try_add_head(two_axioms).forced_propagation, std::nullopt);
    }
};

// ── try_add_head ─────────────────────────────────────────────────────────────

TEST_F(PudMhqTest, AddWithTwoLiveCandidatesHasNoForcedPropagation) {
    EXPECT_CALL(make_head, make(two_axioms)).WillOnce(Return(MockHead{}));
    add_two_live();
}

TEST_F(PudMhqTest, LiveAddStillReturnsItsHeadId) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(live({candidate_a, candidate_b})))
        .WillOnce(Return(live({candidate_c, candidate_d})));
    EXPECT_EQ(mhq.try_add_head(two_axioms).head_id, first_id);
    EXPECT_EQ(mhq.try_add_head(two_axioms).head_id, second_id);
}

TEST_F(PudMhqTest, ResultHeadIdMatchesTheForcedPropagationHeadId) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(refuted()))
        .WillOnce(Return(unit(candidate_a)));
    EXPECT_EQ(mhq.try_add_head(two_axioms).head_id, first_id);
    EXPECT_EQ(mhq.try_add_head({axiom_a}).head_id, second_id);
}

TEST_F(PudMhqTest, LiveForkReturnsTheNewHeadId) {
    add_two_live();
    EXPECT_CALL(ops, resume()).WillOnce(Return(live({110, 120})));
    EXPECT_EQ(mhq.try_fork_head(first_id, two_axioms).head_id, second_id);
}

TEST_F(PudMhqTest, LiveHeadIdCanBeForkedAndRemoved) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(live({candidate_a, candidate_b})));
    const pud_mhq_head_id head_id = mhq.try_add_head(two_axioms).head_id;
    EXPECT_CALL(ops, resume()).WillOnce(Return(unit(110)));
    EXPECT_EQ(mhq.try_fork_head(head_id, two_axioms).forced_propagation, unit_query(second_id));
    mhq.remove_head(head_id);
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), std::nullopt);
}

TEST_F(PudMhqTest, AddResumesTheNewHeadExactlyOnce) {
    EXPECT_CALL(ops, resume()).Times(1).WillOnce(Return(live({candidate_a, candidate_b})));
    mhq.try_add_head(two_axioms);
}

TEST_F(PudMhqTest, AddThatIsImmediatelyUnitIsRemembered) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(unit(candidate_a)));
    EXPECT_EQ(mhq.try_add_head({axiom_a}).forced_propagation, unit_query(first_id));

    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::refuted));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), refuted_query(first_id));
}

TEST_F(PudMhqTest, AddThatIsImmediatelyRefutedIsForgotten) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(refuted()));
    EXPECT_EQ(mhq.try_add_head(two_axioms).forced_propagation, refuted_query(first_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), std::nullopt);
}

TEST_F(PudMhqTest, AddsGetSequentialHeadIds) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(unit(candidate_a)))
        .WillOnce(Return(unit(candidate_b)));
    EXPECT_EQ(mhq.try_add_head({axiom_a}).forced_propagation, unit_query(first_id));
    EXPECT_EQ(mhq.try_add_head({axiom_b}).forced_propagation, unit_query(second_id));
}

TEST_F(PudMhqTest, ImmediatelyRefutedAddStillConsumesAnId) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(refuted()))
        .WillOnce(Return(unit(candidate_a)));
    EXPECT_EQ(mhq.try_add_head(two_axioms).forced_propagation, refuted_query(first_id));
    EXPECT_EQ(mhq.try_add_head({axiom_a}).forced_propagation, unit_query(second_id));
}

// ── candidate_specialization_refuted ─────────────────────────────────────────

TEST_F(PudMhqTest, UnknownCandidateRefutationIsIgnored) {
    EXPECT_EQ(mhq.candidate_specialization_refuted(99), std::nullopt);
}

TEST_F(PudMhqTest, UnknownCandidateRefutationDoesNotTouchHeads) {
    add_two_live();
    EXPECT_CALL(ops, candidate_refuted(_)).Times(0);
    EXPECT_EQ(mhq.candidate_specialization_refuted(99), std::nullopt);
}

TEST_F(PudMhqTest, RefutationIsForwardedToTheHead) {
    add_two_live();
    EXPECT_CALL(ops, candidate_refuted(candidate_b)).WillOnce(Return(pud_forced_unfold_reason::unit));
    mhq.candidate_specialization_refuted(candidate_b);
}

TEST_F(PudMhqTest, RefutationWithNoForcedUnfoldIsNullopt) {
    EXPECT_CALL(ops, resume()).WillOnce(Return(live({candidate_a, candidate_b, candidate_c})));
    mhq.try_add_head(two_axioms);
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(std::nullopt));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), std::nullopt);
}

TEST_F(PudMhqTest, RefutingOneOfTwoLiveCandidatesIsUnit) {
    add_two_live();
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), unit_query(first_id));
}

TEST_F(PudMhqTest, RefutingLastLiveCandidateIsRefuted) {
    add_two_live();
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), unit_query(first_id));

    EXPECT_CALL(ops, candidate_refuted(candidate_b)).WillOnce(Return(pud_forced_unfold_reason::refuted));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_b), refuted_query(first_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_b), std::nullopt);
}

TEST_F(PudMhqTest, SameCandidateRefutedTwiceIsIgnoredTheSecondTime) {
    add_two_live();
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).Times(1).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), unit_query(first_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), std::nullopt);
}

TEST_F(PudMhqTest, UnitHeadStillRoutesItsRemainingCandidate) {
    add_two_live();
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    mhq.candidate_specialization_refuted(candidate_a);
    EXPECT_CALL(ops, candidate_refuted(candidate_b)).WillOnce(Return(pud_forced_unfold_reason::refuted));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_b), refuted_query(first_id));
}

TEST_F(PudMhqTest, RefutationsRouteToTheOwningHead) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(live({candidate_a, candidate_b})))
        .WillOnce(Return(live({candidate_c, candidate_d})));
    mhq.try_add_head(two_axioms);
    mhq.try_add_head(two_axioms);

    EXPECT_CALL(ops, candidate_refuted(candidate_c)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_c), unit_query(second_id));
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), unit_query(first_id));
}

TEST_F(PudMhqTest, RefutingOneHeadLeavesTheOtherRouted) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(unit(candidate_a)))
        .WillOnce(Return(live({candidate_c, candidate_d})));
    mhq.try_add_head({axiom_a});
    mhq.try_add_head(two_axioms);

    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::refuted));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), refuted_query(first_id));
    EXPECT_CALL(ops, candidate_refuted(candidate_d)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_d), unit_query(second_id));
}

// ── remove_head ──────────────────────────────────────────────────────────────

TEST_F(PudMhqTest, RemoveUnlinksCandidates) {
    add_two_live();
    mhq.remove_head(first_id);
    EXPECT_CALL(ops, candidate_refuted(_)).Times(0);
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_b), std::nullopt);
}

TEST_F(PudMhqTest, RemoveOfUnknownIdIsNoop) {
    add_two_live();
    mhq.remove_head(first_id + 100);
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), unit_query(first_id));
}

TEST_F(PudMhqTest, RemoveTwiceIsNoop) {
    add_two_live();
    mhq.remove_head(first_id);
    mhq.remove_head(first_id);
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), std::nullopt);
}

TEST_F(PudMhqTest, RemoveOfUnitHeadUnlinksItsRemainingCandidate) {
    add_two_live();
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    mhq.candidate_specialization_refuted(candidate_a);
    mhq.remove_head(first_id);
    EXPECT_CALL(ops, candidate_refuted(_)).Times(0);
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_b), std::nullopt);
}

TEST_F(PudMhqTest, RemoveOfRefutedHeadIsNoop) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(refuted()))
        .WillOnce(Return(live({candidate_c, candidate_d})));
    mhq.try_add_head(two_axioms);
    mhq.try_add_head(two_axioms);
    mhq.remove_head(first_id);
    EXPECT_CALL(ops, candidate_refuted(candidate_c)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_c), unit_query(second_id));
}

TEST_F(PudMhqTest, RemoveOneHeadLeavesOthersRouted) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(live({candidate_a, candidate_b})))
        .WillOnce(Return(live({candidate_c, candidate_d})));
    mhq.try_add_head(two_axioms);
    mhq.try_add_head(two_axioms);
    mhq.remove_head(first_id);

    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), std::nullopt);
    EXPECT_CALL(ops, candidate_refuted(candidate_c)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_c), unit_query(second_id));
}

// ── try_fork_head ────────────────────────────────────────────────────────────

TEST_F(PudMhqTest, ForkWithTwoLiveCandidatesHasNoForcedPropagation) {
    add_two_live();
    EXPECT_CALL(fork_head, fork(_, two_axioms)).WillOnce(Return(MockHead{}));
    EXPECT_CALL(ops, resume()).WillOnce(Return(live({110, 120})));
    EXPECT_EQ(mhq.try_fork_head(first_id, two_axioms).forced_propagation, std::nullopt);
}

TEST_F(PudMhqTest, ForkResumesTheNewHeadExactlyOnce) {
    add_two_live();
    EXPECT_CALL(ops, resume()).Times(1).WillOnce(Return(live({110, 120})));
    mhq.try_fork_head(first_id, two_axioms);
}

TEST_F(PudMhqTest, ForkDoesNotMakeANewHead) {
    add_two_live();
    EXPECT_CALL(make_head, make(_)).Times(0);
    EXPECT_CALL(ops, resume()).WillOnce(Return(live({110, 120})));
    mhq.try_fork_head(first_id, two_axioms);
}

TEST_F(PudMhqTest, ForkThatIsImmediatelyUnitIsRemembered) {
    add_two_live();
    EXPECT_CALL(ops, resume()).WillOnce(Return(unit(110)));
    EXPECT_EQ(mhq.try_fork_head(first_id, two_axioms).forced_propagation, unit_query(second_id));

    EXPECT_CALL(ops, candidate_refuted(110)).WillOnce(Return(pud_forced_unfold_reason::refuted));
    EXPECT_EQ(mhq.candidate_specialization_refuted(110), refuted_query(second_id));
}

TEST_F(PudMhqTest, ForkThatIsImmediatelyRefutedIsForgotten) {
    add_two_live();
    EXPECT_CALL(ops, resume()).WillOnce(Return(refuted()));
    EXPECT_EQ(mhq.try_fork_head(first_id, two_axioms).forced_propagation, refuted_query(second_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(110), std::nullopt);
}

TEST_F(PudMhqTest, ImmediatelyRefutedForkLeavesParentRouted) {
    add_two_live();
    EXPECT_CALL(ops, resume()).WillOnce(Return(refuted()));
    mhq.try_fork_head(first_id, two_axioms);
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), unit_query(first_id));
}

TEST_F(PudMhqTest, ForkedCandidatesRouteToTheChild) {
    add_two_live();
    EXPECT_CALL(ops, resume()).WillOnce(Return(live({110, 120})));
    mhq.try_fork_head(first_id, two_axioms);

    EXPECT_CALL(ops, candidate_refuted(120)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(120), unit_query(second_id));
    EXPECT_CALL(ops, candidate_refuted(candidate_b)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_b), unit_query(first_id));
}

TEST_F(PudMhqTest, RemoveParentLeavesForkedChildRouted) {
    add_two_live();
    EXPECT_CALL(ops, resume()).WillOnce(Return(live({110, 120})));
    mhq.try_fork_head(first_id, two_axioms);
    mhq.remove_head(first_id);

    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), std::nullopt);
    EXPECT_CALL(ops, candidate_refuted(110)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(110), unit_query(second_id));
}

TEST_F(PudMhqTest, RemoveChildLeavesParentRouted) {
    add_two_live();
    EXPECT_CALL(ops, resume()).WillOnce(Return(live({110, 120})));
    mhq.try_fork_head(first_id, two_axioms);
    mhq.remove_head(second_id);

    EXPECT_EQ(mhq.candidate_specialization_refuted(110), std::nullopt);
    EXPECT_CALL(ops, candidate_refuted(candidate_a)).WillOnce(Return(pud_forced_unfold_reason::unit));
    EXPECT_EQ(mhq.candidate_specialization_refuted(candidate_a), unit_query(first_id));
}

TEST_F(PudMhqTest, ForkOfForkGetsTheNextId) {
    add_two_live();
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(live({110, 120})))
        .WillOnce(Return(unit(210)));
    mhq.try_fork_head(first_id, two_axioms);
    EXPECT_EQ(mhq.try_fork_head(second_id, two_axioms).forced_propagation, unit_query(third_id));
}

TEST_F(PudMhqTest, ForkOfUnitHeadIsAllowed) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(unit(candidate_a)))
        .WillOnce(Return(unit(110)));
    mhq.try_add_head({axiom_a});
    EXPECT_EQ(mhq.try_fork_head(first_id, {axiom_a}).forced_propagation, unit_query(second_id));
}

}  // namespace
