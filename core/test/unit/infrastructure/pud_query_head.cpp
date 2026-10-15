#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_head.hpp"
#include "infrastructure/pud_query_head_factory.hpp"
#include "infrastructure/pud_query_head_forker.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {

struct descent_t {
    pud_node_id node;
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

using test_factory_t = pud_query_head_factory<
    descent_t,
    MockTryAddCandidateSpecializationHead,
    MockTryForkCandidateSpecializationHead>;

using test_forker_t = pud_query_head_forker<
    descent_t,
    MockTryAddCandidateSpecializationHead,
    MockTryForkCandidateSpecializationHead>;

struct PudQueryHeadTest : public ::testing::Test {
    NiceMock<MockTryAddCandidateSpecializationHead> try_add;
    NiceMock<MockTryForkCandidateSpecializationHead> try_fork;
    descent_t axiom_a{1};
    descent_t axiom_b{2};
    descent_t axiom_c{3};
    pud_mhcs_head_id candidate_a = 10;
    pud_mhcs_head_id candidate_b = 20;
    pud_mhcs_head_id candidate_c = 30;
    pud_mhcs_head_id forked_a = 110;
    pud_mhcs_head_id forked_b = 120;

    test_query_head_t make_head(const std::vector<descent_t>& roots) {
        return test_query_head_t{try_add, try_fork, roots};
    }
};

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

TEST_F(PudQueryHeadTest, TwoSuccessfulAddsHasNoForcedUnfold) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto head = make_head({axiom_a, axiom_b});
    auto context = head.resume();
    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(candidate_a, candidate_b));
    EXPECT_EQ(context.forced_unfold_reason, std::nullopt);
}

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

TEST_F(PudQueryHeadTest, ForkForwardsOnlyLiveCandidates) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(axiom_c)).WillOnce(Return(candidate_c));
    auto parent = make_head({axiom_a, axiom_b, axiom_c});

    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_c, axiom_c)).WillOnce(Return(forked_b));
    test_query_head_t child{parent, {axiom_a, axiom_b, axiom_c}};
    auto context = child.resume();

    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(forked_a, forked_b));
    EXPECT_EQ(context.forced_unfold_reason, std::nullopt);
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

TEST_F(PudQueryHeadTest, FactoryMakeDispatchesTryAdd) {
    test_factory_t factory{try_add, try_fork};
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto head = factory.make({axiom_a, axiom_b});
    auto context = head.resume();
    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(candidate_a, candidate_b));
}

TEST_F(PudQueryHeadTest, ForkerDispatchesTryFork) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto parent = make_head({axiom_a, axiom_b});

    test_forker_t forker;
    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(forked_a));
    EXPECT_CALL(try_fork, try_fork_head(candidate_b, axiom_b)).WillOnce(Return(forked_b));
    auto child = forker.fork(parent, {axiom_a, axiom_b});
    auto context = child.resume();
    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(forked_a, forked_b));
}

}  // namespace
