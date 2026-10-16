#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_query_head_factory.hpp"

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::UnorderedElementsAre;

namespace {

struct descent_t {
    pud_node_id at;
    pud_node_id node() const { return at; }
    bool operator==(const descent_t&) const = default;
};

struct MockTryAddCandidateSpecializationHead {
    MOCK_METHOD((std::optional<pud_mhcs_head_id>), try_add_head, (descent_t));
};

struct MockTryForkCandidateSpecializationHead {
    MOCK_METHOD((std::optional<pud_mhcs_head_id>), try_fork_head, (pud_mhcs_head_id, descent_t));
};

using test_factory_t = pud_query_head_factory<
    descent_t,
    MockTryAddCandidateSpecializationHead,
    MockTryForkCandidateSpecializationHead>;

struct PudQueryHeadFactoryTest : public ::testing::Test {
    NiceMock<MockTryAddCandidateSpecializationHead> try_add;
    NiceMock<MockTryForkCandidateSpecializationHead> try_fork;
    test_factory_t factory{try_add, try_fork};
    descent_t axiom_a{1};
    descent_t axiom_b{2};
    pud_mhcs_head_id candidate_a = 10;
    pud_mhcs_head_id candidate_b = 20;
};

TEST_F(PudQueryHeadFactoryTest, MakeTriesEachRoot) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto head = factory.make({axiom_a, axiom_b});
    auto context = head.resume();
    EXPECT_THAT(context.candidate_ids, UnorderedElementsAre(candidate_a, candidate_b));
    EXPECT_EQ(context.forced_unfold_reason, std::nullopt);
}

TEST_F(PudQueryHeadFactoryTest, MakeWithNoRootsIsRefuted) {
    auto head = factory.make({});
    EXPECT_EQ(head.resume().forced_unfold_reason, pud_forced_unfold_reason::refuted);
}

TEST_F(PudQueryHeadFactoryTest, MakeWithOneLiveRootIsUnit) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(std::nullopt));
    EXPECT_CALL(try_add, try_add_head(axiom_b)).WillOnce(Return(candidate_b));
    auto head = factory.make({axiom_a, axiom_b});
    EXPECT_EQ(head.resume().forced_unfold_reason, pud_forced_unfold_reason::unit);
}

TEST_F(PudQueryHeadFactoryTest, MadeHeadsAreIndependent) {
    EXPECT_CALL(try_add, try_add_head(axiom_a))
        .WillOnce(Return(candidate_a))
        .WillOnce(Return(110));
    EXPECT_CALL(try_add, try_add_head(axiom_b))
        .WillOnce(Return(candidate_b))
        .WillOnce(Return(120));
    auto first = factory.make({axiom_a, axiom_b});
    auto second = factory.make({axiom_a, axiom_b});

    first.candidate_refuted(candidate_a);

    EXPECT_THAT(first.resume().candidate_ids, UnorderedElementsAre(candidate_b));
    EXPECT_THAT(second.resume().candidate_ids, UnorderedElementsAre(110, 120));
}

TEST_F(PudQueryHeadFactoryTest, MadeHeadForksThroughTheInjectedForker) {
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    auto parent = factory.make({axiom_a});
    EXPECT_CALL(try_fork, try_fork_head(candidate_a, axiom_a)).WillOnce(Return(110));
    test_factory_t::head_type child{parent, {axiom_a}};
    EXPECT_THAT(child.resume().candidate_ids, UnorderedElementsAre(110));
}

TEST_F(PudQueryHeadFactoryTest, MakeNeverForks) {
    StrictMock<MockTryForkCandidateSpecializationHead> strict_try_fork;
    pud_query_head_factory<descent_t, MockTryAddCandidateSpecializationHead,
        StrictMock<MockTryForkCandidateSpecializationHead>> strict_factory{try_add, strict_try_fork};
    EXPECT_CALL(try_add, try_add_head(axiom_a)).WillOnce(Return(candidate_a));
    strict_factory.make({axiom_a});
}

}  // namespace
