#include <deque>
#include <optional>
#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_unit_propagator.hpp"
#include "value_objects/pud_mhcs_head_id.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockFindQueryHolder {
    MOCK_METHOD(std::optional<pud_unfold>, find_holder, (pud_mhq_head_id), (const));
};
struct MockGetQueryHolders {
    MOCK_METHOD(std::vector<pud_global_node_id>, get_holders, (pud_mhq_head_id), (const));
};
struct MockGetQueryCandidates {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, candidates, (pud_mhq_head_id), (const));
};
struct MockGetLiveNodes {
    MOCK_METHOD(const std::set<pud_global_node_id>&, live, (), (const));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};
struct MockAllowUnfold {
    MOCK_METHOD(bool, allow, (pud_global_node_id), (const));
};
struct MockUnfoldLeaf {
    MOCK_METHOD(pud_unfold_outcome, unfold, (pud_global_node_id, pud_body_goal_id));
};

using unit_queries_t = std::deque<pud_mhq_head_id>;
using test_propagator_t = pud_unit_propagator<
    unit_queries_t, NiceMock<MockFindQueryHolder>, NiceMock<MockGetQueryHolders>, NiceMock<MockGetQueryCandidates>,
    NiceMock<MockGetLiveNodes>, NiceMock<MockIsNodeRefuted>, NiceMock<MockAllowUnfold>, NiceMock<MockUnfoldLeaf>>;

// Query 40 is unit and held on goal 2 by leaves 7 and 8, both live, unrefuted and unfoldable.
struct PudUnitPropagatorTest : public ::testing::Test {
    unit_queries_t unit_queries{40};
    NiceMock<MockFindQueryHolder> holder;
    NiceMock<MockGetQueryHolders> holders;
    NiceMock<MockGetQueryCandidates> candidates;
    NiceMock<MockGetLiveNodes> frontier;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockAllowUnfold> allower;
    NiceMock<MockUnfoldLeaf> unfolder;
    test_propagator_t propagator{unit_queries, holder, holders, candidates, frontier, refuted, allower, unfolder};
    std::set<pud_global_node_id> live{pud_test_node(7), pud_test_node(8)};

    void SetUp() override {
        ON_CALL(holder, find_holder(40)).WillByDefault(Return(pud_unfold{.leaf = pud_test_node(7), .goal = 2}));
        ON_CALL(holders, get_holders(40))
            .WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(7), pud_test_node(8)}));
        ON_CALL(candidates, candidates(40)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30}));
        ON_CALL(frontier, live()).WillByDefault(ReturnRef(live));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
        ON_CALL(allower, allow(_)).WillByDefault(Return(true));
        ON_CALL(unfolder, unfold(_, _)).WillByDefault(Return(pud_unfold_outcome::on_focus));
    }
};

}

TEST_F(PudUnitPropagatorTest, AnEmptyQueueUnfoldsNothing) {
    unit_queries.clear();
    EXPECT_CALL(unfolder, unfold(_, _)).Times(0);
    EXPECT_EQ(propagator.propagate(), pud_unfold_outcome::none);
}

TEST_F(PudUnitPropagatorTest, TheFirstReadyHolderIsUnfoldedOnTheHeldGoal) {
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 2)).WillOnce(Return(pud_unfold_outcome::off_focus));
    EXPECT_EQ(propagator.propagate(), pud_unfold_outcome::off_focus);
}

TEST_F(PudUnitPropagatorTest, AQueryWithMoreReadyHoldersReturnsToTheFrontOfTheQueue) {
    unit_queries.push_back(41);
    propagator.propagate();
    EXPECT_THAT(unit_queries, ElementsAre(40, 41));
}

TEST_F(PudUnitPropagatorTest, AQueryWithOneReadyHolderLeavesTheQueue) {
    ON_CALL(holders, get_holders(40)).WillByDefault(Return(std::vector<pud_global_node_id>{pud_test_node(7)}));
    propagator.propagate();
    EXPECT_THAT(unit_queries, IsEmpty());
}

TEST_F(PudUnitPropagatorTest, AnUnheldOrNoLongerUnitQueryIsDroppedAndTheNextIsTried) {
    unit_queries = {39, 41, 40};
    ON_CALL(holder, find_holder(39)).WillByDefault(Return(std::nullopt));
    ON_CALL(holder, find_holder(41)).WillByDefault(Return(pud_unfold{.leaf = pud_test_node(7), .goal = 3}));
    ON_CALL(candidates, candidates(41)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30, 31}));
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 2));
    propagator.propagate();
}

TEST_F(PudUnitPropagatorTest, AHolderOffTheFrontierRefutedOrNotUnfoldableIsSkipped) {
    live.erase(pud_test_node(7));
    EXPECT_CALL(unfolder, unfold(pud_test_node(8), 2));
    propagator.propagate();
    live.insert(pud_test_node(7));
    unit_queries = {40};
    ON_CALL(refuted, is_refuted(pud_test_node(7))).WillByDefault(Return(true));
    EXPECT_CALL(unfolder, unfold(pud_test_node(8), 2));
    propagator.propagate();
    ON_CALL(refuted, is_refuted(pud_test_node(7))).WillByDefault(Return(false));
    unit_queries = {40};
    ON_CALL(allower, allow(pud_test_node(7))).WillByDefault(Return(false));
    EXPECT_CALL(unfolder, unfold(pud_test_node(8), 2));
    propagator.propagate();
}

TEST_F(PudUnitPropagatorTest, AQueryWithNoReadyHolderIsDropped) {
    ON_CALL(allower, allow(_)).WillByDefault(Return(false));
    EXPECT_CALL(unfolder, unfold(_, _)).Times(0);
    EXPECT_EQ(propagator.propagate(), pud_unfold_outcome::none);
    EXPECT_THAT(unit_queries, IsEmpty());
}
