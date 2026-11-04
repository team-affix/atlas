#include <deque>
#include <map>
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
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockFindQueryLeaves {
    MOCK_METHOD((coroutine<pud_unfold, void>), find_leaves, (pud_mhq_head_id), (const));
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
    unit_queries_t, NiceMock<MockFindQueryLeaves>, NiceMock<MockGetQueryCandidates>, NiceMock<MockGetLiveNodes>,
    NiceMock<MockIsNodeRefuted>, NiceMock<MockAllowUnfold>, NiceMock<MockUnfoldLeaf>>;

// Query 40 is unit and held on goal 2 by leaves 7 and 8, both live, unrefuted and unfoldable. A query no leaf
// holds is found on no leaf.
struct PudUnitPropagatorTest : public ::testing::Test {
    unit_queries_t unit_queries{40};
    NiceMock<MockFindQueryLeaves> leaf_finder;
    NiceMock<MockGetQueryCandidates> candidates;
    NiceMock<MockGetLiveNodes> frontier;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockAllowUnfold> allower;
    NiceMock<MockUnfoldLeaf> unfolder;
    test_propagator_t propagator{unit_queries, leaf_finder, candidates, frontier, refuted, allower, unfolder};
    std::set<pud_global_node_id> live{pud_test_node(7), pud_test_node(8)};
    std::map<pud_mhq_head_id, std::vector<pud_unfold>> held{
        {40, {pud_unfold{.leaf = pud_test_node(7), .goal = 2}, pud_unfold{.leaf = pud_test_node(8), .goal = 2}}}};

    coroutine<pud_unfold, void> find_held_leaves(pud_mhq_head_id head) {
        const auto holding = held.find(head);
        if (holding == held.end())
            co_return;
        for (const pud_unfold leaf : holding->second)
            co_yield leaf;
    }

    void SetUp() override {
        ON_CALL(leaf_finder, find_leaves(_)).WillByDefault(Invoke(this, &PudUnitPropagatorTest::find_held_leaves));
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

TEST_F(PudUnitPropagatorTest, TheFirstReadyLeafIsUnfoldedOnTheHeldGoal) {
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 2)).WillOnce(Return(pud_unfold_outcome::off_focus));
    EXPECT_EQ(propagator.propagate(), pud_unfold_outcome::off_focus);
}

TEST_F(PudUnitPropagatorTest, AQueryWithMoreReadyLeavesReturnsToTheFrontOfTheQueue) {
    unit_queries.push_back(41);
    propagator.propagate();
    EXPECT_THAT(unit_queries, ElementsAre(40, 41));
}

TEST_F(PudUnitPropagatorTest, AQueryWithOneReadyLeafLeavesTheQueue) {
    held.at(40).pop_back();
    propagator.propagate();
    EXPECT_THAT(unit_queries, IsEmpty());
}

TEST_F(PudUnitPropagatorTest, AnUnheldOrNoLongerUnitQueryIsDroppedAndTheNextIsTried) {
    unit_queries = {39, 41, 40};
    held.emplace(41, std::vector<pud_unfold>{pud_unfold{.leaf = pud_test_node(7), .goal = 3}});
    ON_CALL(candidates, candidates(41)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30, 31}));
    EXPECT_CALL(candidates, candidates(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(candidates, candidates(39)).Times(0);
    EXPECT_CALL(unfolder, unfold(pud_test_node(7), 2));
    propagator.propagate();
}

TEST_F(PudUnitPropagatorTest, ALeafOffTheFrontierRefutedOrNotUnfoldableIsSkipped) {
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

TEST_F(PudUnitPropagatorTest, AQueryWithNoReadyLeafIsDropped) {
    ON_CALL(allower, allow(_)).WillByDefault(Return(false));
    EXPECT_CALL(unfolder, unfold(_, _)).Times(0);
    EXPECT_EQ(propagator.propagate(), pud_unfold_outcome::none);
    EXPECT_THAT(unit_queries, IsEmpty());
}

TEST_F(PudUnitPropagatorTest, TheLeavesAreFoundOnlyAsFarAsTheSecondReadyLeaf) {
    held.at(40).push_back(pud_unfold{.leaf = pud_test_node(9), .goal = 2});
    EXPECT_CALL(allower, allow(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(allower, allow(pud_test_node(9))).Times(0);
    propagator.propagate();
    EXPECT_THAT(unit_queries, ElementsAre(40));
}
