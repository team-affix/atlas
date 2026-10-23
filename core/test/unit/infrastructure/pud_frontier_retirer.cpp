#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_frontier_retirer.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {

struct MockMarkNodeState {
    MOCK_METHOD(void, mark_refuted, (pud_node_id));
};
struct MockPropagateRefutation {
    MOCK_METHOD(void, refute, (pud_node_id));
};
struct MockReleaseLeaf {
    MOCK_METHOD(void, release, (pud_node_id));
};
struct MockInvalidateQueryLeaves {
    MOCK_METHOD(std::vector<pud_mhq_head_id>, invalidate_leaves, (const std::vector<pud_node_id>&));
};
struct MockGetQueryHolders {
    MOCK_METHOD(std::vector<pud_node_id>, get_holders, (pud_mhq_head_id));
};

using test_retirer_t = pud_frontier_retirer<
    NiceMock<MockMarkNodeState>, NiceMock<MockPropagateRefutation>, NiceMock<MockReleaseLeaf>,
    NiceMock<MockInvalidateQueryLeaves>, NiceMock<MockGetQueryHolders>>;

struct PudFrontierRetirerTest : public ::testing::Test {
    NiceMock<MockMarkNodeState> states;
    NiceMock<MockPropagateRefutation> propagator;
    NiceMock<MockReleaseLeaf> releaser;
    NiceMock<MockInvalidateQueryLeaves> queries;
    NiceMock<MockGetQueryHolders> holders;
    test_retirer_t retirer{states, propagator, releaser, queries, holders};
};

}

TEST_F(PudFrontierRetirerTest, AnExpandedLeafIsReleasedAndInvalidatedButNotRefuted) {
    EXPECT_CALL(states, mark_refuted(_)).Times(0);
    EXPECT_CALL(releaser, release(4));
    EXPECT_CALL(queries, invalidate_leaves(ElementsAre(4u)));
    EXPECT_THAT(retirer.retire({4}, {}), IsEmpty());
}

TEST_F(PudFrontierRetirerTest, ARefutedLeafIsMarkedAndPropagatedBeforeAnyLeafIsInvalidated) {
    InSequence in_order;
    EXPECT_CALL(states, mark_refuted(6));
    EXPECT_CALL(propagator, refute(6));
    EXPECT_CALL(releaser, release(4));
    EXPECT_CALL(releaser, release(6));
    EXPECT_CALL(queries, invalidate_leaves(ElementsAre(4u, 6u)));
    EXPECT_THAT(retirer.retire({4}, {6}), ElementsAre(6u));
}

TEST_F(PudFrontierRetirerTest, AQueryLeftWithoutCandidatesRefutesEveryLeafStillHoldingIt) {
    ON_CALL(queries, invalidate_leaves(ElementsAre(4u))).WillByDefault(Return(std::vector<pud_mhq_head_id>{30, 31}));
    ON_CALL(holders, get_holders(30)).WillByDefault(Return(std::vector<pud_node_id>{7, 8}));
    ON_CALL(holders, get_holders(31)).WillByDefault(Return(std::vector<pud_node_id>{8}));
    EXPECT_CALL(queries, invalidate_leaves(ElementsAre(4u)));
    EXPECT_CALL(states, mark_refuted(7));
    EXPECT_CALL(states, mark_refuted(8));
    EXPECT_CALL(queries, invalidate_leaves(ElementsAre(7u, 8u)));
    EXPECT_THAT(retirer.retire({4}, {}), UnorderedElementsAre(7u, 8u));
}
