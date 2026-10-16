#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_head_forker.hpp"

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

using test_forker_t = pud_query_head_forker<
    descent_t,
    MockTryAddCandidateSpecializationHead,
    MockTryForkCandidateSpecializationHead>;

struct PudQueryHeadForkerTest : public ::testing::Test {
    NiceMock<MockTryAddCandidateSpecializationHead> try_add;
    NiceMock<MockTryForkCandidateSpecializationHead> try_fork;
    test_forker_t forker;
    descent_t axiom_a{1, 0};
    descent_t axiom_b{2, 0};
    pud_mhcs_head_id candidate_a = 10;
    pud_mhcs_head_id candidate_b = 20;
    pud_mhcs_head_id forked_a = 110;
    pud_mhcs_head_id forked_b = 120;

    test_forker_t::head_type make_parent() {
        EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
        EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
        return test_forker_t::head_type{try_add, try_fork, {axiom_a, axiom_b}};
    }
};

TEST_F(PudQueryHeadForkerTest, ForkForksEachLiveCandidate) {
    auto parent = make_parent();
    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(forked_b));
    auto child = forker.fork(parent, {axiom_a, axiom_b});
    EXPECT_THAT(child.resume().candidate_ids, UnorderedElementsAre(forked_a, forked_b));
}

TEST_F(PudQueryHeadForkerTest, ForkPassesTheNewRootDescents) {
    auto parent = make_parent();
    const descent_t specialized_a{axiom_a.node(), 3};
    const descent_t specialized_b{axiom_b.node(), 4};
    EXPECT_CALL(try_fork, try_fork_head(candidate_a, specialized_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, specialized_b)).WillOnce(Return(forked_b));
    forker.fork(parent, {specialized_a, specialized_b});
}

TEST_F(PudQueryHeadForkerTest, ForkLeavesParentUntouched) {
    auto parent = make_parent();
    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(std::nullopt));
    auto child = forker.fork(parent, {axiom_a, axiom_b});
    EXPECT_EQ(child.resume().forced_unfold_reason, pud_forced_unfold_reason::refuted);
    EXPECT_THAT(parent.resume().candidate_ids, UnorderedElementsAre(candidate_a, candidate_b));
}

TEST_F(PudQueryHeadForkerTest, ForkReportsUnitWhenOneSurvives) {
    auto parent = make_parent();
    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(std::nullopt));
    auto child = forker.fork(parent, {axiom_a, axiom_b});
    EXPECT_EQ(child.resume().forced_unfold_reason, pud_forced_unfold_reason::unit);
}

TEST_F(PudQueryHeadForkerTest, ForkNeverAdds) {
    StrictMock<MockTryAddCandidateSpecializationHead> strict_try_add;
    EXPECT_CALL(strict_try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    pud_query_head_forker<descent_t, StrictMock<MockTryAddCandidateSpecializationHead>,
        MockTryForkCandidateSpecializationHead> strict_forker;
    decltype(strict_forker)::head_type parent{strict_try_add, try_fork, {axiom_a}};
    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    strict_forker.fork(parent, {axiom_a, axiom_b});
}

}  // namespace
