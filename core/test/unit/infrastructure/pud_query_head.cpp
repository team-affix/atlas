#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_head.hpp"

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::UnorderedElementsAre;

namespace {

struct descent_t {
    pud_node_id at;
    pud_node_id node() const { return at; }
    int version;
    bool operator==(const descent_t&) const = default;
};

struct MockTryAddCandidateSpecializationHead {
    MOCK_METHOD((std::optional<pud_mhcs_head_id>), try_add_head, (descent_t));
};

struct MockTryForkCandidateSpecializationHead {
    MOCK_METHOD((std::optional<pud_mhcs_head_id>), try_fork_head, (pud_mhcs_head_id, descent_t));
};

using test_query_head_t = pud_query_head<
    descent_t,
    MockTryAddCandidateSpecializationHead,
    MockTryForkCandidateSpecializationHead>;

struct PudQueryHeadTest : public ::testing::Test {
    NiceMock<MockTryAddCandidateSpecializationHead> try_add;
    NiceMock<MockTryForkCandidateSpecializationHead> try_fork;
    descent_t axiom_a{1, 0};
    descent_t axiom_b{2, 0};
    descent_t axiom_c{3, 0};
    descent_t axiom_d{4, 0};
    pud_mhcs_head_id candidate_a = 10;
    pud_mhcs_head_id candidate_b = 20;
    pud_mhcs_head_id candidate_c = 30;
    pud_mhcs_head_id forked_a = 110;
    pud_mhcs_head_id forked_b = 120;
    pud_mhcs_head_id forked_c = 130;

    test_query_head_t make_head(const std::vector<descent_t>& roots) {
        return test_query_head_t{try_add, try_fork, roots};
    }

    test_query_head_t make_three_live() {
        EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
        EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
        EXPECT_CALL(try_add, try_add_head(axiom_c)).WillOnce(Return(candidate_c));
        return make_head({axiom_a, axiom_b, axiom_c});
    }
};

// ── make constructor ─────────────────────────────────────────────────────────

TEST_F(PudQueryHeadTest, EmptyRootsIsRefuted) {
    auto head = make_head({});
    auto context = head.resume();
    EXPECT_TRUE(context.candidate_ids.empty());
    EXPECT_EQ(context.forced_unfold_reason, pud_forced_unfold_reason::refuted);
}

TEST_F(PudQueryHeadTest, AllAddsFailIsRefuted) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(std::nullopt));
    auto head = make_head({axiom_a, axiom_b});
    auto context = head.resume();
    EXPECT_TRUE(context.candidate_ids.empty());
    EXPECT_EQ(context.forced_unfold_reason, pud_forced_unfold_reason::refuted);
}

TEST_F(PudQueryHeadTest, SoleSuccessfulAddIsUnit) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    auto head = make_head({axiom_a});
    auto context = head.resume();
    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(candidate_a));
    EXPECT_EQ(context.forced_unfold_reason, pud_forced_unfold_reason::unit);
}

TEST_F(PudQueryHeadTest, OneOfTwoSuccessfulAddsIsUnit) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(std::nullopt));
    auto head = make_head({axiom_a, axiom_b});
    auto context = head.resume();
    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(candidate_a));
    EXPECT_EQ(context.forced_unfold_reason, pud_forced_unfold_reason::unit);
}

TEST_F(PudQueryHeadTest, LastOfThreeSuccessfulAddsAloneIsUnit) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(axiom_c)).WillOnce(Return(candidate_c));
    auto head = make_head({axiom_a, axiom_b, axiom_c});
    auto context = head.resume();
    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(candidate_c));
    EXPECT_EQ(context.forced_unfold_reason, pud_forced_unfold_reason::unit);
}

TEST_F(PudQueryHeadTest, TwoSuccessfulAddsHasNoForcedUnfold) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto head = make_head({axiom_a, axiom_b});
    auto context = head.resume();
    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(candidate_a, candidate_b));
    EXPECT_EQ(context.forced_unfold_reason, std::nullopt);
}

TEST_F(PudQueryHeadTest, ThreeSuccessfulAddsHasNoForcedUnfold) {
    auto head = make_three_live();
    auto context = head.resume();
    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(candidate_a, candidate_b, candidate_c));
    EXPECT_EQ(context.forced_unfold_reason, std::nullopt);
}

TEST_F(PudQueryHeadTest, MakeTriesEveryRootInOrder) {
    StrictMock<MockTryAddCandidateSpecializationHead> strict_try_add;
    {
        InSequence in_order;
        EXPECT_CALL(strict_try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
        EXPECT_CALL(strict_try_add, try_add_head(axiom_b)).WillOnce(Return(std::nullopt));
        EXPECT_CALL(strict_try_add, try_add_head(axiom_c)).WillOnce(Return(candidate_c));
    }
    pud_query_head<descent_t, StrictMock<MockTryAddCandidateSpecializationHead>,
        MockTryForkCandidateSpecializationHead> head{strict_try_add, try_fork, {axiom_a, axiom_b, axiom_c}};
}

TEST_F(PudQueryHeadTest, MakeKeepsTryingAfterAFailedAdd) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    EXPECT_CALL(try_add, try_add_head(axiom_c)).WillOnce(Return(candidate_c));
    auto head = make_head({axiom_a, axiom_b, axiom_c});
    EXPECT_THAT(head.resume().candidate_ids, UnorderedElementsAre(candidate_b, candidate_c));
}

TEST_F(PudQueryHeadTest, MakeNeverForks) {
    StrictMock<MockTryForkCandidateSpecializationHead> strict_try_fork;
    EXPECT_CALL(try_add, try_add_head(_)).WillRepeatedly(Return(candidate_a));
    pud_query_head<descent_t, MockTryAddCandidateSpecializationHead,
        StrictMock<MockTryForkCandidateSpecializationHead>> head{try_add, strict_try_fork, {axiom_a}};
}

// ── resume ───────────────────────────────────────────────────────────────────

TEST_F(PudQueryHeadTest, ResumeIsRepeatable) {
    auto head = make_three_live();
    auto first = head.resume();
    auto second = head.resume();
    EXPECT_THAT(first.candidate_ids, UnorderedElementsAre(candidate_a, candidate_b, candidate_c));
    EXPECT_THAT(second.candidate_ids, UnorderedElementsAre(candidate_a, candidate_b, candidate_c));
    EXPECT_EQ(first.forced_unfold_reason, second.forced_unfold_reason);
}

TEST_F(PudQueryHeadTest, ResumeAfterRefutationOmitsRefutedCandidate) {
    auto head = make_three_live();
    head.candidate_refuted(candidate_b);
    EXPECT_THAT(head.resume().candidate_ids, UnorderedElementsAre(candidate_a, candidate_c));
}

TEST_F(PudQueryHeadTest, ResumeAfterRefutationReportsUnit) {
    auto head = make_three_live();
    head.candidate_refuted(candidate_a);
    head.candidate_refuted(candidate_c);
    auto context = head.resume();
    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(candidate_b));
    EXPECT_EQ(context.forced_unfold_reason, pud_forced_unfold_reason::unit);
}

TEST_F(PudQueryHeadTest, ResumeAfterAllRefutedReportsRefuted) {
    auto head = make_three_live();
    head.candidate_refuted(candidate_a);
    head.candidate_refuted(candidate_b);
    head.candidate_refuted(candidate_c);
    auto context = head.resume();
    EXPECT_TRUE(context.candidate_ids.empty());
    EXPECT_EQ(context.forced_unfold_reason, pud_forced_unfold_reason::refuted);
}

// ── candidate_refuted ────────────────────────────────────────────────────────

TEST_F(PudQueryHeadTest, RefutingOneOfTwoLiveCandidatesIsUnit) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto head = make_head({axiom_a, axiom_b});
    EXPECT_EQ(head.candidate_refuted(candidate_a), pud_forced_unfold_reason::unit);
}

TEST_F(PudQueryHeadTest, RefutingLastLiveCandidateIsRefuted) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto head = make_head({axiom_a, axiom_b});
    EXPECT_EQ(head.candidate_refuted(candidate_a), pud_forced_unfold_reason::unit);
    EXPECT_EQ(head.candidate_refuted(candidate_b), pud_forced_unfold_reason::refuted);
}

TEST_F(PudQueryHeadTest, RefutingOneOfThreeHasNoForcedUnfold) {
    auto head = make_three_live();
    EXPECT_EQ(head.candidate_refuted(candidate_b), std::nullopt);
}

TEST_F(PudQueryHeadTest, RefutingInAnyOrderEndsRefuted) {
    auto head = make_three_live();
    EXPECT_EQ(head.candidate_refuted(candidate_c), std::nullopt);
    EXPECT_EQ(head.candidate_refuted(candidate_a), pud_forced_unfold_reason::unit);
    EXPECT_EQ(head.candidate_refuted(candidate_b), pud_forced_unfold_reason::refuted);
}

TEST_F(PudQueryHeadTest, RefutingSoleCandidateIsRefuted) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    auto head = make_head({axiom_a});
    EXPECT_EQ(head.candidate_refuted(candidate_a), pud_forced_unfold_reason::refuted);
}

// ── forking constructor ──────────────────────────────────────────────────────

TEST_F(PudQueryHeadTest, ForkForwardsOnlyLiveCandidates) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(axiom_c)).WillOnce(Return(candidate_c));
    auto parent = make_head({axiom_a, axiom_b, axiom_c});

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_c, axiom_c)).WillOnce(Return(forked_c));
    test_query_head_t child{parent, {axiom_a, axiom_b, axiom_c}};
    auto context = child.resume();

    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(forked_a, forked_c));
    EXPECT_EQ(context.forced_unfold_reason, std::nullopt);
}

TEST_F(PudQueryHeadTest, ForkPairsCandidatesByRootNotPosition) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto parent = make_head({axiom_a, axiom_b});

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(forked_b));
    test_query_head_t child{parent, {axiom_b, axiom_a}};
    auto context = child.resume();

    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(forked_a, forked_b));
}

TEST_F(PudQueryHeadTest, ForkPassesTheNewDescentNotTheParentDescent) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    auto parent = make_head({axiom_a});

    const descent_t specialized_a{axiom_a.node(), 7};
    EXPECT_CALL(try_fork, try_fork_head(candidate_a, specialized_a)).WillOnce(Return(forked_a));
    test_query_head_t child{parent, {specialized_a}};
    EXPECT_THAT(child.resume().candidate_ids, UnorderedElementsAre(forked_a));
}

TEST_F(PudQueryHeadTest, RefutedCandidateIsNotForked) {
    auto parent = make_three_live();
    parent.candidate_refuted(candidate_b);

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_c, axiom_c)).WillOnce(Return(forked_c));
    test_query_head_t child{parent, {axiom_a, axiom_b, axiom_c}};

    EXPECT_THAT(child.resume().candidate_ids, UnorderedElementsAre(forked_a, forked_c));
}

TEST_F(PudQueryHeadTest, ForkNeverTriesToAddDeadAxioms) {
    StrictMock<MockTryAddCandidateSpecializationHead> strict_try_add;
    EXPECT_CALL(strict_try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(strict_try_add, try_add_head(axiom_b)).WillOnce(Return(std::nullopt));
    pud_query_head<descent_t, StrictMock<MockTryAddCandidateSpecializationHead>,
        MockTryForkCandidateSpecializationHead> parent{strict_try_add, try_fork, {axiom_a, axiom_b}};

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    decltype(parent) child{parent, {axiom_a, axiom_b}};
    EXPECT_THAT(child.resume().candidate_ids, UnorderedElementsAre(forked_a));
}

TEST_F(PudQueryHeadTest, ForkSkipsRootsTheParentNeverHad) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto parent = make_head({axiom_a, axiom_b});

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(forked_b));
    EXPECT_CALL(try_fork, try_fork_head(_, axiom_d)).Times(0);
    test_query_head_t child{parent, {axiom_a, axiom_b, axiom_d}};
    EXPECT_THAT(child.resume().candidate_ids, UnorderedElementsAre(forked_a, forked_b));
}

TEST_F(PudQueryHeadTest, ForkWithSubsetOfRootsForksOnlyThoseGiven) {
    auto parent = make_three_live();

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, _)).Times(0);
    EXPECT_CALL(try_fork, try_fork_head(candidate_c, axiom_c)).WillOnce(Return(forked_c));
    test_query_head_t child{parent, {axiom_a, axiom_c}};
    EXPECT_THAT(child.resume().candidate_ids, UnorderedElementsAre(forked_a, forked_c));
}

TEST_F(PudQueryHeadTest, ForkWithNoRootsIsRefuted) {
    auto parent = make_three_live();
    test_query_head_t child{parent, {}};
    auto context = child.resume();
    EXPECT_TRUE(context.candidate_ids.empty());
    EXPECT_EQ(context.forced_unfold_reason, pud_forced_unfold_reason::refuted);
}

TEST_F(PudQueryHeadTest, ForkOfFullyRefutedParentIsRefutedWithoutForking) {
    StrictMock<MockTryForkCandidateSpecializationHead> strict_try_fork;
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    pud_query_head<descent_t, MockTryAddCandidateSpecializationHead,
        StrictMock<MockTryForkCandidateSpecializationHead>> parent{try_add, strict_try_fork, {axiom_a}};
    parent.candidate_refuted(candidate_a);

    decltype(parent) child{parent, {axiom_a}};
    EXPECT_EQ(child.resume().forced_unfold_reason, pud_forced_unfold_reason::refuted);
}

TEST_F(PudQueryHeadTest, ForkThatLosesAllButOneCandidateIsUnit) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto parent = make_head({axiom_a, axiom_b});

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(std::nullopt));
    test_query_head_t child{parent, {axiom_a, axiom_b}};
    auto context = child.resume();

    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(forked_a));
    EXPECT_EQ(context.forced_unfold_reason, pud_forced_unfold_reason::unit);
}

TEST_F(PudQueryHeadTest, ForkThatLosesEveryCandidateIsRefuted) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto parent = make_head({axiom_a, axiom_b});

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(std::nullopt));
    test_query_head_t child{parent, {axiom_a, axiom_b}};
    auto context = child.resume();

    EXPECT_TRUE(context.candidate_ids.empty());
    EXPECT_EQ(context.forced_unfold_reason, pud_forced_unfold_reason::refuted);
}

TEST_F(PudQueryHeadTest, ForkedChildRefutationsDoNotTouchParent) {
    auto parent = make_three_live();
    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(forked_b));
    EXPECT_CALL(try_fork, try_fork_head(candidate_c, axiom_c)).WillOnce(Return(forked_c));
    test_query_head_t child{parent, {axiom_a, axiom_b, axiom_c}};

    child.candidate_refuted(forked_a);
    child.candidate_refuted(forked_b);

    EXPECT_THAT(parent.resume().candidate_ids, UnorderedElementsAre(candidate_a, candidate_b, candidate_c));
    EXPECT_EQ(child.resume().forced_unfold_reason, pud_forced_unfold_reason::unit);
}

TEST_F(PudQueryHeadTest, ParentRefutationsAfterForkDoNotTouchChild) {
    auto parent = make_three_live();
    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(forked_b));
    EXPECT_CALL(try_fork, try_fork_head(candidate_c, axiom_c)).WillOnce(Return(forked_c));
    test_query_head_t child{parent, {axiom_a, axiom_b, axiom_c}};

    parent.candidate_refuted(candidate_a);
    parent.candidate_refuted(candidate_b);

    EXPECT_THAT(child.resume().candidate_ids, UnorderedElementsAre(forked_a, forked_b, forked_c));
    EXPECT_EQ(parent.resume().forced_unfold_reason, pud_forced_unfold_reason::unit);
}

TEST_F(PudQueryHeadTest, ForkOfForkForksTheChildIds) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto parent = make_head({axiom_a, axiom_b});

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(forked_b));
    test_query_head_t child{parent, {axiom_a, axiom_b}};

    const pud_mhcs_head_id grand_a = 210;
    const pud_mhcs_head_id grand_b = 220;
    EXPECT_CALL(try_fork, try_fork_head(forked_a, axiom_a)).WillOnce(Return(grand_a));
    EXPECT_CALL(try_fork, try_fork_head(forked_b, axiom_b)).WillOnce(Return(grand_b));
    test_query_head_t grandchild{child, {axiom_a, axiom_b}};

    EXPECT_THAT(grandchild.resume().candidate_ids, UnorderedElementsAre(grand_a, grand_b));
}

TEST_F(PudQueryHeadTest, ForkOfUnitChildIsUnitOrRefuted) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto parent = make_head({axiom_a, axiom_b});
    parent.candidate_refuted(candidate_b);

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    test_query_head_t unit_child{parent, {axiom_a, axiom_b}};
    EXPECT_EQ(unit_child.resume().forced_unfold_reason, pud_forced_unfold_reason::unit);

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(std::nullopt));
    test_query_head_t refuted_child{parent, {axiom_a, axiom_b}};
    EXPECT_EQ(refuted_child.resume().forced_unfold_reason, pud_forced_unfold_reason::refuted);
}

TEST_F(PudQueryHeadTest, ForkedChildRefutesThroughItsOwnIds) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto parent = make_head({axiom_a, axiom_b});

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(forked_b));
    test_query_head_t child{parent, {axiom_a, axiom_b}};

    EXPECT_EQ(child.candidate_refuted(forked_b), pud_forced_unfold_reason::unit);
    EXPECT_EQ(child.candidate_refuted(forked_a), pud_forced_unfold_reason::refuted);
}

}  // namespace
