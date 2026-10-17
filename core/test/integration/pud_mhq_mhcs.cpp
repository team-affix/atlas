#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_query_head.hpp"
#include "infrastructure/pud_query_head_factory.hpp"
#include "infrastructure/pud_query_head_forker.hpp"
#include "infrastructure/pud_candidate_specialization_head.hpp"
#include "infrastructure/pud_witness_search_head.hpp"

using ::testing::_;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {

struct descent_t {
    pud_node_id at;
    pud_node_id node() const { return at; }
    int version;
    bool operator==(const descent_t&) const = default;
};

using candidate_context_t = pud_candidate_resume_context<descent_t>;

struct CandidateHeadOps {
    MOCK_METHOD((std::optional<candidate_context_t>), resume, ());
    MOCK_METHOD(void, witness_refuted, (pud_mhws_head_id));
};

struct MockCandidateHead {
    static CandidateHeadOps* ops;
    std::optional<candidate_context_t> resume() { return ops->resume(); }
    void witness_refuted(pud_mhws_head_id witness_id) { ops->witness_refuted(witness_id); }
};

CandidateHeadOps* MockCandidateHead::ops = nullptr;

struct MockMakeCandidateHead {
    MOCK_METHOD(MockCandidateHead, make, (descent_t));
};

struct MockForkCandidateHead {
    MOCK_METHOD(MockCandidateHead, fork, (const MockCandidateHead&, descent_t));
};

struct MockRemoveWitnessHead {
    MOCK_METHOD(void, remove_head, (pud_mhws_head_id));
};

using mhcs_t = pud_mhcs<descent_t, MockCandidateHead, MockMakeCandidateHead, MockForkCandidateHead, NiceMock<MockRemoveWitnessHead>>;
using query_head_t = pud_query_head<descent_t, mhcs_t, mhcs_t>;
using query_head_factory_t = pud_query_head_factory<query_head_t, descent_t, mhcs_t, mhcs_t>;
using query_head_forker_t = pud_query_head_forker<query_head_t, descent_t>;
using mhq_t = pud_mhq<descent_t, query_head_t, query_head_factory_t, query_head_forker_t, mhcs_t>;

using propagation_t = std::optional<pud_forced_query_propagation>;

struct PudMhqMhcsIntegrationTest : public ::testing::Test {
    NiceMock<CandidateHeadOps> ops;
    NiceMock<MockMakeCandidateHead> make_candidate;
    NiceMock<MockForkCandidateHead> fork_candidate;
    NiceMock<MockRemoveWitnessHead> remove_witness_head;
    mhcs_t mhcs{make_candidate, fork_candidate, remove_witness_head};
    query_head_factory_t query_factory{mhcs, mhcs};
    query_head_forker_t query_forker;
    mhq_t mhq{query_factory, query_forker, mhcs};
    descent_t axiom_a{1, 0};
    descent_t axiom_b{2, 0};
    std::vector<descent_t> two_axioms{axiom_a, axiom_b};
    pud_node_id leaf_a = 100;
    pud_node_id leaf_b = 200;
    pud_node_id leaf_c = 300;
    pud_mhq_head_id first_query = 0;
    pud_mhq_head_id second_query = 1;

    void SetUp() override {
        MockCandidateHead::ops = &ops;
        ON_CALL(make_candidate, make(_)).WillByDefault(Return(MockCandidateHead{}));
        ON_CALL(fork_candidate, fork(_, _)).WillByDefault(Return(MockCandidateHead{}));
    }

    static candidate_context_t self_at(pud_node_id leaf, descent_t descent) {
        return {.justification = pud_candidate_self_witness{.node = leaf}, .descent = descent};
    }

    static candidate_context_t choice_between(pud_mhws_head_id witness_a, pud_mhws_head_id witness_b, descent_t descent) {
        return {
            .justification = pud_candidate_choice_point{.witness_a = witness_a, .witness_b = witness_b},
            .descent = descent,
        };
    }

    static propagation_t unit_query(pud_mhq_head_id head_id) {
        return pud_unit_query{.head_id = head_id};
    }

    static propagation_t refuted_query(pud_mhq_head_id head_id) {
        return pud_refuted_query{.head_id = head_id};
    }

    std::vector<propagation_t> invalidate(pud_node_id leaf) {
        std::vector<propagation_t> propagations;
        for (pud_mhcs_head_id candidate_id : mhcs.invalidate_leaf(leaf))
            propagations.push_back(mhq.candidate_specialization_refuted(candidate_id));
        return propagations;
    }

    void add_two_self_witnessed() {
        EXPECT_CALL(ops, resume())
            .WillOnce(Return(self_at(leaf_a, axiom_a)))
            .WillOnce(Return(self_at(leaf_b, axiom_b)));
        ASSERT_EQ(mhq.try_add_head(two_axioms).forced_propagation, std::nullopt);
    }
};

// ── adding ───────────────────────────────────────────────────────────────────

TEST_F(PudMhqMhcsIntegrationTest, EachAxiomDispatchesACandidateSpecialization) {
    EXPECT_CALL(make_candidate, make(axiom_a)).WillOnce(Return(MockCandidateHead{}));
    EXPECT_CALL(make_candidate, make(axiom_b)).WillOnce(Return(MockCandidateHead{}));
    add_two_self_witnessed();
}

TEST_F(PudMhqMhcsIntegrationTest, UnjustifiedAxiomIsDeadAtAdd) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_at(leaf_a, axiom_a)))
        .WillOnce(Return(std::nullopt));
    EXPECT_EQ(mhq.try_add_head(two_axioms).forced_propagation, unit_query(first_query));
}

TEST_F(PudMhqMhcsIntegrationTest, NoJustifiedAxiomIsRefutedAtAdd) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));
    EXPECT_EQ(mhq.try_add_head(two_axioms).forced_propagation, refuted_query(first_query));
}

// ── leaf invalidation ────────────────────────────────────────────────────────

TEST_F(PudMhqMhcsIntegrationTest, LosingOneWitnessMakesQueryUnit) {
    add_two_self_witnessed();
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    EXPECT_THAT(invalidate(leaf_a), UnorderedElementsAre(unit_query(first_query)));
}

TEST_F(PudMhqMhcsIntegrationTest, LosingBothWitnessesMakesQueryRefuted) {
    add_two_self_witnessed();
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));
    EXPECT_THAT(invalidate(leaf_a), UnorderedElementsAre(unit_query(first_query)));
    EXPECT_THAT(invalidate(leaf_b), UnorderedElementsAre(refuted_query(first_query)));
}

TEST_F(PudMhqMhcsIntegrationTest, ReplacementWitnessKeepsQueryLive) {
    add_two_self_witnessed();
    EXPECT_CALL(ops, resume()).WillOnce(Return(self_at(leaf_c, axiom_a)));
    EXPECT_TRUE(invalidate(leaf_a).empty());

    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    EXPECT_THAT(invalidate(leaf_c), UnorderedElementsAre(unit_query(first_query)));
}

TEST_F(PudMhqMhcsIntegrationTest, InvalidatingAnUnrelatedLeafChangesNothing) {
    add_two_self_witnessed();
    EXPECT_TRUE(invalidate(leaf_c).empty());
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    EXPECT_THAT(invalidate(leaf_b), UnorderedElementsAre(unit_query(first_query)));
}

// ── witness refutation ───────────────────────────────────────────────────────

TEST_F(PudMhqMhcsIntegrationTest, ChoicePointCollapseMakesQueryUnit) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(choice_between(7, 8, axiom_a)))
        .WillOnce(Return(self_at(leaf_b, axiom_b)));
    ASSERT_EQ(mhq.try_add_head(two_axioms).forced_propagation, std::nullopt);

    EXPECT_CALL(ops, witness_refuted(7u));
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    std::vector<pud_mhcs_head_id> lost = mhcs.witnesses_refuted({7});
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(mhq.candidate_specialization_refuted(lost.front()), unit_query(first_query));
}

TEST_F(PudMhqMhcsIntegrationTest, ChoicePointThatRecoversKeepsQueryLive) {
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(choice_between(7, 8, axiom_a)))
        .WillOnce(Return(self_at(leaf_b, axiom_b)));
    ASSERT_EQ(mhq.try_add_head(two_axioms).forced_propagation, std::nullopt);

    EXPECT_CALL(ops, resume()).WillOnce(Return(choice_between(9, 8, axiom_a)));
    EXPECT_TRUE(mhcs.witnesses_refuted({7}).empty());

    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    std::vector<pud_mhcs_head_id> lost = mhcs.witnesses_refuted({9});
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(mhq.candidate_specialization_refuted(lost.front()), unit_query(first_query));
}

// ── forking ──────────────────────────────────────────────────────────────────

TEST_F(PudMhqMhcsIntegrationTest, ForkForksEveryCandidateSpecialization) {
    add_two_self_witnessed();
    const descent_t specialized_a{axiom_a.node(), 4};
    const descent_t specialized_b{axiom_b.node(), 4};
    EXPECT_CALL(fork_candidate, fork(_, specialized_a)).WillOnce(Return(MockCandidateHead{}));
    EXPECT_CALL(fork_candidate, fork(_, specialized_b)).WillOnce(Return(MockCandidateHead{}));
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_at(leaf_a, specialized_a)))
        .WillOnce(Return(self_at(leaf_b, specialized_b)));
    EXPECT_EQ(mhq.try_fork_head(first_query, {specialized_a, specialized_b}).forced_propagation, std::nullopt);
}

TEST_F(PudMhqMhcsIntegrationTest, SharedLeafInvalidationReachesParentAndChild) {
    add_two_self_witnessed();
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_at(leaf_a, axiom_a)))
        .WillOnce(Return(self_at(leaf_b, axiom_b)));
    ASSERT_EQ(mhq.try_fork_head(first_query, two_axioms).forced_propagation, std::nullopt);

    EXPECT_CALL(ops, resume())
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));
    EXPECT_THAT(invalidate(leaf_a),
        UnorderedElementsAre(unit_query(first_query), unit_query(second_query)));
}

TEST_F(PudMhqMhcsIntegrationTest, ForkWhoseCandidateCannotForkIsUnit) {
    add_two_self_witnessed();
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_at(leaf_a, axiom_a)))
        .WillOnce(Return(std::nullopt));
    EXPECT_EQ(mhq.try_fork_head(first_query, two_axioms).forced_propagation, unit_query(second_query));
}

TEST_F(PudMhqMhcsIntegrationTest, ForkWhoseCandidatesAllFailIsRefutedAndParentSurvives) {
    add_two_self_witnessed();
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));
    EXPECT_EQ(mhq.try_fork_head(first_query, two_axioms).forced_propagation, refuted_query(second_query));

    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    EXPECT_THAT(invalidate(leaf_a), UnorderedElementsAre(unit_query(first_query)));
}

TEST_F(PudMhqMhcsIntegrationTest, ForkSkipsAxiomsAlreadyDeadInParent) {
    add_two_self_witnessed();
    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    ASSERT_THAT(invalidate(leaf_a), UnorderedElementsAre(unit_query(first_query)));

    EXPECT_CALL(fork_candidate, fork(_, axiom_a)).Times(0);
    EXPECT_CALL(fork_candidate, fork(_, axiom_b)).WillOnce(Return(MockCandidateHead{}));
    EXPECT_CALL(ops, resume()).WillOnce(Return(self_at(leaf_b, axiom_b)));
    EXPECT_EQ(mhq.try_fork_head(first_query, two_axioms).forced_propagation, unit_query(second_query));
}

// ── removing ─────────────────────────────────────────────────────────────────

TEST_F(PudMhqMhcsIntegrationTest, RemovedQueryRemovesItsCandidateHeads) {
    add_two_self_witnessed();
    mhq.remove_head(first_query);
    EXPECT_CALL(ops, resume()).Times(0);
    EXPECT_THAT(invalidate(leaf_a), IsEmpty());
    EXPECT_THAT(invalidate(leaf_b), IsEmpty());
}

TEST_F(PudMhqMhcsIntegrationTest, RemovedChildLeavesParentReachable) {
    add_two_self_witnessed();
    EXPECT_CALL(ops, resume())
        .WillOnce(Return(self_at(leaf_a, axiom_a)))
        .WillOnce(Return(self_at(leaf_b, axiom_b)));
    mhq.try_fork_head(first_query, two_axioms);
    mhq.remove_head(second_query);

    EXPECT_CALL(ops, resume()).WillOnce(Return(std::nullopt));
    EXPECT_THAT(invalidate(leaf_a), UnorderedElementsAre(unit_query(first_query)));
}

}  // namespace
