#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_query_head.hpp"
#include "infrastructure/pud_query_head_factory.hpp"
#include "infrastructure/pud_query_head_forker.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

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

using query_head_t = pud_query_head<
    descent_t, MockTryAddCandidateSpecializationHead, MockTryForkCandidateSpecializationHead>;
using query_head_factory_t = pud_query_head_factory<
    descent_t, MockTryAddCandidateSpecializationHead, MockTryForkCandidateSpecializationHead>;
using query_head_forker_t = pud_query_head_forker<
    descent_t, MockTryAddCandidateSpecializationHead, MockTryForkCandidateSpecializationHead>;
struct MockRemoveCandidateHead {
    MOCK_METHOD(void, remove_head, (pud_mhcs_head_id));
};

using mhq_t = pud_mhq<descent_t, query_head_t, query_head_factory_t, query_head_forker_t, ::testing::NiceMock<MockRemoveCandidateHead>>;

using propagation_t = std::optional<pud_forced_query_propagation>;

struct PudMhqQueryHeadIntegrationTest : public ::testing::Test {
    NiceMock<MockTryAddCandidateSpecializationHead> try_add;
    NiceMock<MockTryForkCandidateSpecializationHead> try_fork;
    query_head_factory_t factory{try_add, try_fork};
    query_head_forker_t forker;
    ::testing::NiceMock<MockRemoveCandidateHead> remove_candidate_head;
    mhq_t mhq{factory, forker, remove_candidate_head};
    descent_t axiom_a{1, 0};
    descent_t axiom_b{2, 0};
    descent_t axiom_c{3, 0};
    std::vector<descent_t> three_axioms{axiom_a, axiom_b, axiom_c};
    std::vector<descent_t> two_axioms{axiom_a, axiom_b};
    pud_mhq_head_id first_id = 0;
    pud_mhq_head_id second_id = 1;
    pud_mhq_head_id third_id = 2;

    static propagation_t unit_query(pud_mhq_head_id head_id) {
        return pud_unit_query{.head_id = head_id};
    }

    static propagation_t refuted_query(pud_mhq_head_id head_id) {
        return pud_refuted_query{.head_id = head_id};
    }

    void expect_add(descent_t root, std::optional<pud_mhcs_head_id> candidate_id) {
        EXPECT_CALL(try_add, try_add_head(root)).WillOnce(Return(candidate_id));
    }

    void expect_fork(pud_mhcs_head_id parent_id, descent_t root, std::optional<pud_mhcs_head_id> candidate_id) {
        EXPECT_CALL(try_fork, try_fork_head(parent_id, root)).WillOnce(Return(candidate_id));
    }

    void add_three_live() {
        expect_add(axiom_a, 10);
        expect_add(axiom_b, 20);
        expect_add(axiom_c, 30);
        ASSERT_EQ(mhq.try_add_head(three_axioms).forced_propagation, std::nullopt);
    }
};

// ── adding ───────────────────────────────────────────────────────────────────

TEST_F(PudMhqQueryHeadIntegrationTest, RefutationsWalkThroughNoneThenUnitThenRefuted) {
    add_three_live();
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(20), unit_query(first_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(30), refuted_query(first_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, RefutationOrderDoesNotMatter) {
    add_three_live();
    EXPECT_EQ(mhq.candidate_specialization_refuted(30), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), unit_query(first_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(20), refuted_query(first_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, AddWithOneLiveAxiomIsUnitImmediately) {
    expect_add(axiom_a, std::nullopt);
    expect_add(axiom_b, 20);
    expect_add(axiom_c, std::nullopt);
    EXPECT_EQ(mhq.try_add_head(three_axioms).forced_propagation, unit_query(first_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(20), refuted_query(first_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, AddWithNoLiveAxiomIsRefutedImmediately) {
    expect_add(axiom_a, std::nullopt);
    expect_add(axiom_b, std::nullopt);
    EXPECT_EQ(mhq.try_add_head(two_axioms).forced_propagation, refuted_query(first_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, AddWithNoAxiomsIsRefutedImmediately) {
    EXPECT_EQ(mhq.try_add_head({}).forced_propagation, refuted_query(first_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, RefutedQueryIgnoresFurtherRefutations) {
    add_three_live();
    mhq.candidate_specialization_refuted(10);
    mhq.candidate_specialization_refuted(20);
    mhq.candidate_specialization_refuted(30);
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(20), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(30), std::nullopt);
}

TEST_F(PudMhqQueryHeadIntegrationTest, RepeatedRefutationOfSameCandidateIsIgnored) {
    add_three_live();
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(20), unit_query(first_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, TwoQueriesDoNotInterfere) {
    expect_add(axiom_a, 10);
    expect_add(axiom_b, 20);
    mhq.try_add_head(two_axioms);
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(11));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(21));
    mhq.try_add_head(two_axioms);

    EXPECT_EQ(mhq.candidate_specialization_refuted(11), unit_query(second_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), unit_query(first_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(21), refuted_query(second_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(20), refuted_query(first_id));
}

// ── removing ─────────────────────────────────────────────────────────────────

TEST_F(PudMhqQueryHeadIntegrationTest, RemovedQueryIgnoresRefutations) {
    add_three_live();
    mhq.remove_head(first_id);
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(20), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(30), std::nullopt);
}

TEST_F(PudMhqQueryHeadIntegrationTest, RemovedUnitQueryIgnoresItsLastCandidate) {
    add_three_live();
    mhq.candidate_specialization_refuted(10);
    ASSERT_EQ(mhq.candidate_specialization_refuted(20), unit_query(first_id));
    mhq.remove_head(first_id);
    EXPECT_EQ(mhq.candidate_specialization_refuted(30), std::nullopt);
}

TEST_F(PudMhqQueryHeadIntegrationTest, RemovingOneQueryLeavesTheOther) {
    expect_add(axiom_a, 10);
    expect_add(axiom_b, 20);
    mhq.try_add_head(two_axioms);
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(11));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(21));
    mhq.try_add_head(two_axioms);

    mhq.remove_head(first_id);
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(11), unit_query(second_id));
}

// ── forking ──────────────────────────────────────────────────────────────────

TEST_F(PudMhqQueryHeadIntegrationTest, ForkForksEveryLiveCandidate) {
    add_three_live();
    expect_fork(10, axiom_a, 110);
    expect_fork(20, axiom_b, 120);
    expect_fork(30, axiom_c, 130);
    EXPECT_EQ(mhq.try_fork_head(first_id, three_axioms).forced_propagation, std::nullopt);
}

TEST_F(PudMhqQueryHeadIntegrationTest, ForkSkipsCandidatesRefutedBeforeTheFork) {
    add_three_live();
    mhq.candidate_specialization_refuted(20);
    expect_fork(10, axiom_a, 110);
    expect_fork(30, axiom_c, 130);
    EXPECT_CALL(try_fork, try_fork_head(20, _)).Times(0);
    EXPECT_EQ(mhq.try_fork_head(first_id, three_axioms).forced_propagation, std::nullopt);
}

TEST_F(PudMhqQueryHeadIntegrationTest, ForkPassesSpecializedDescents) {
    add_three_live();
    const descent_t specialized_a{axiom_a.node(), 5};
    const descent_t specialized_b{axiom_b.node(), 5};
    const descent_t specialized_c{axiom_c.node(), 5};
    expect_fork(10, specialized_a, 110);
    expect_fork(20, specialized_b, 120);
    expect_fork(30, specialized_c, 130);
    mhq.try_fork_head(first_id, {specialized_a, specialized_b, specialized_c});
}

TEST_F(PudMhqQueryHeadIntegrationTest, ForkThatKeepsOneCandidateIsUnit) {
    add_three_live();
    expect_fork(10, axiom_a, std::nullopt);
    expect_fork(20, axiom_b, 120);
    expect_fork(30, axiom_c, std::nullopt);
    EXPECT_EQ(mhq.try_fork_head(first_id, three_axioms).forced_propagation, unit_query(second_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(120), refuted_query(second_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, ForkThatKeepsNoCandidateIsRefutedAndParentSurvives) {
    add_three_live();
    expect_fork(10, axiom_a, std::nullopt);
    expect_fork(20, axiom_b, std::nullopt);
    expect_fork(30, axiom_c, std::nullopt);
    EXPECT_EQ(mhq.try_fork_head(first_id, three_axioms).forced_propagation, refuted_query(second_id));

    EXPECT_EQ(mhq.candidate_specialization_refuted(10), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(20), unit_query(first_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, ChildAndParentRefuteIndependently) {
    add_three_live();
    expect_fork(10, axiom_a, 110);
    expect_fork(20, axiom_b, 120);
    expect_fork(30, axiom_c, 130);
    mhq.try_fork_head(first_id, three_axioms);

    EXPECT_EQ(mhq.candidate_specialization_refuted(110), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(120), unit_query(second_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(130), refuted_query(second_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(30), unit_query(first_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, ParentRefutedAfterForkLeavesChildLive) {
    add_three_live();
    expect_fork(10, axiom_a, 110);
    expect_fork(20, axiom_b, 120);
    expect_fork(30, axiom_c, 130);
    mhq.try_fork_head(first_id, three_axioms);

    mhq.candidate_specialization_refuted(10);
    mhq.candidate_specialization_refuted(20);
    EXPECT_EQ(mhq.candidate_specialization_refuted(30), refuted_query(first_id));

    EXPECT_EQ(mhq.candidate_specialization_refuted(110), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(120), unit_query(second_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, RemoveParentLeavesChildRouted) {
    add_three_live();
    expect_fork(10, axiom_a, 110);
    expect_fork(20, axiom_b, 120);
    expect_fork(30, axiom_c, 130);
    mhq.try_fork_head(first_id, three_axioms);
    mhq.remove_head(first_id);

    EXPECT_EQ(mhq.candidate_specialization_refuted(10), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(110), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(120), unit_query(second_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, RemoveChildLeavesParentRouted) {
    add_three_live();
    expect_fork(10, axiom_a, 110);
    expect_fork(20, axiom_b, 120);
    expect_fork(30, axiom_c, 130);
    mhq.try_fork_head(first_id, three_axioms);
    mhq.remove_head(second_id);

    EXPECT_EQ(mhq.candidate_specialization_refuted(110), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), std::nullopt);
    EXPECT_EQ(mhq.candidate_specialization_refuted(20), unit_query(first_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, ForkOfForkRoutesToTheGrandchild) {
    expect_add(axiom_a, 10);
    expect_add(axiom_b, 20);
    mhq.try_add_head(two_axioms);
    expect_fork(10, axiom_a, 110);
    expect_fork(20, axiom_b, 120);
    mhq.try_fork_head(first_id, two_axioms);
    expect_fork(110, axiom_a, 210);
    expect_fork(120, axiom_b, 220);
    EXPECT_EQ(mhq.try_fork_head(second_id, two_axioms).forced_propagation, std::nullopt);

    EXPECT_EQ(mhq.candidate_specialization_refuted(210), unit_query(third_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(110), unit_query(second_id));
    EXPECT_EQ(mhq.candidate_specialization_refuted(10), unit_query(first_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, ForkOfUnitQueryForksOnlyItsLastCandidate) {
    add_three_live();
    mhq.candidate_specialization_refuted(10);
    ASSERT_EQ(mhq.candidate_specialization_refuted(30), unit_query(first_id));

    expect_fork(20, axiom_b, 120);
    EXPECT_CALL(try_fork, try_fork_head(10, _)).Times(0);
    EXPECT_CALL(try_fork, try_fork_head(30, _)).Times(0);
    EXPECT_EQ(mhq.try_fork_head(first_id, three_axioms).forced_propagation, unit_query(second_id));
}

TEST_F(PudMhqQueryHeadIntegrationTest, HeadIdsAreSequentialAcrossAddsAndForks) {
    expect_add(axiom_a, 10);
    EXPECT_EQ(mhq.try_add_head({axiom_a}).forced_propagation, unit_query(first_id));
    expect_fork(10, axiom_a, 110);
    EXPECT_EQ(mhq.try_fork_head(first_id, {axiom_a}).forced_propagation, unit_query(second_id));
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(11));
    EXPECT_EQ(mhq.try_add_head({axiom_a}).forced_propagation, unit_query(third_id));
}

}  // namespace
