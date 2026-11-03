#include <limits>
#include <optional>
#include <set>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_choice_lister.hpp"
#include "value_objects/pud_node_queries.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockGetQueryCandidates {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, candidates, (pud_mhq_head_id), (const));
};
struct MockFindCandidateWitnesses {
    MOCK_METHOD(const pud_candidate_witnesses*, find, (pud_mhcs_head_id), (const));
};
struct MockFindWitnessNode {
    MOCK_METHOD(std::optional<pud_global_node_id>, node_of, (pud_mhws_head_id), (const));
};
struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id), (const));
};
struct MockGetLiveNodes {
    MOCK_METHOD(const std::set<pud_global_node_id>&, live, (), (const));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};
struct MockGetNodeQueries {
    MOCK_METHOD(const pud_node_queries&, get, (pud_global_node_id), (const));
};
struct MockIsQueryExhausted {
    MOCK_METHOD(bool, is_exhausted, (pud_mhq_head_id), (const));
};
struct MockAllowUnfold {
    MOCK_METHOD(bool, allow, (pud_global_node_id), (const));
};

using test_choice_lister_t = pud_lodestar_choice_lister<
    NiceMock<MockGetQueryCandidates>, NiceMock<MockFindCandidateWitnesses>, NiceMock<MockFindWitnessNode>,
    NiceMock<MockContainsFocusNode>, NiceMock<MockGetLiveNodes>, NiceMock<MockIsNodeRefuted>,
    NiceMock<MockGetNodeQueries>, NiceMock<MockIsQueryExhausted>, NiceMock<MockAllowUnfold>>;

pud_lodestar_choice choice(
    pud_leaf_goal_query query,
    pud_mhcs_head_id candidate,
    pud_lodestar_side side,
    pud_global_node_id leaf,
    pud_body_goal_id goal) {
    return pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = query.node, .query_goal = query.goal, .candidate = candidate, .side = side},
        .leaf = leaf,
        .body_goal = goal,
    };
}

// Conjecture leaf 3 asks query 40 on its goal 0. Candidate 30 rests on witness searches 50 and 51,
// whose leaves 10 and 11 lie outside the conjecture module; candidate 31 is its own witness at leaf
// 12, inside it. Every leaf is live, unrefuted, may be unfolded, and holds goal 5 with candidates.
struct PudLodestarChoiceListerTest : public ::testing::Test {
    NiceMock<MockGetQueryCandidates> candidates;
    NiceMock<MockFindCandidateWitnesses> witness_index;
    NiceMock<MockFindWitnessNode> witnesses;
    NiceMock<MockContainsFocusNode> focus;
    NiceMock<MockGetLiveNodes> frontier;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockGetNodeQueries> node_queries;
    NiceMock<MockIsQueryExhausted> exhausted;
    NiceMock<MockAllowUnfold> allower;
    test_choice_lister_t lister{
        candidates, witness_index, witnesses, focus, frontier, refuted, node_queries, exhausted, allower};
    pud_leaf_goal_query query{.node = pud_test_node(3), .goal = 0, .head = 40, .has_candidates = true};
    pud_candidate_witnesses witnessed{
        .self_witness = false, .self_leaf = {}, .witnesses = {50, 51}};
    pud_candidate_witnesses self_witnessed{
        .self_witness = true, .self_leaf = pud_test_node(12), .witnesses = {}};
    std::set<pud_global_node_id> live{pud_test_node(3), pud_test_node(10), pud_test_node(11), pud_test_node(12)};
    pud_node_queries ready_on_goal_five{pud_node_queries{}.set(5, 60)};
    pud_lodestar_choice caller = choice(
        query, std::numeric_limits<pud_mhcs_head_id>::max(), pud_lodestar_side::caller, pud_test_node(3), 0);

    void SetUp() override {
        ON_CALL(candidates, candidates(40)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30, 31}));
        ON_CALL(witness_index, find(30)).WillByDefault(Return(&witnessed));
        ON_CALL(witness_index, find(31)).WillByDefault(Return(&self_witnessed));
        ON_CALL(witnesses, node_of(50)).WillByDefault(Return(pud_test_node(10)));
        ON_CALL(witnesses, node_of(51)).WillByDefault(Return(pud_test_node(11)));
        ON_CALL(focus, contains(_)).WillByDefault(Return(false));
        ON_CALL(focus, contains(pud_test_node(3))).WillByDefault(Return(true));
        ON_CALL(focus, contains(pud_test_node(12))).WillByDefault(Return(true));
        ON_CALL(frontier, live()).WillByDefault(ReturnRef(live));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
        ON_CALL(node_queries, get(_)).WillByDefault(ReturnRef(ready_on_goal_five));
        ON_CALL(exhausted, is_exhausted(_)).WillByDefault(Return(false));
        ON_CALL(allower, allow(_)).WillByDefault(Return(true));
    }
};

}

TEST_F(PudLodestarChoiceListerTest, TheOuterRingHoldsTheWitnessLeavesOutsideTheConjectureModuleBySide) {
    EXPECT_EQ(lister.list(query, false), (std::vector<pud_lodestar_choice>{
        choice(query, 30, pud_lodestar_side::a, pud_test_node(10), 5),
        choice(query, 30, pud_lodestar_side::b, pud_test_node(11), 5),
    }));
}

TEST_F(PudLodestarChoiceListerTest, TheFocusRingHoldsTheWitnessLeavesInsideItAndOneCallerSlotPerQuery) {
    EXPECT_EQ(lister.list(query, true), (std::vector<pud_lodestar_choice>{
        choice(query, 31, pud_lodestar_side::self, pud_test_node(12), 5),
        caller,
    }));
}

TEST_F(PudLodestarChoiceListerTest, AQueryWithoutCandidatesAddsNothingNotEvenACallerSlot) {
    query.has_candidates = false;
    EXPECT_CALL(candidates, candidates(_)).Times(0);
    EXPECT_THAT(lister.list(query, true), IsEmpty());
}

TEST_F(PudLodestarChoiceListerTest, AnUnwatchedCandidateOrAWitnessWithoutALeafAddsNothing) {
    ON_CALL(witness_index, find(31)).WillByDefault(Return(nullptr));
    ON_CALL(witnesses, node_of(50)).WillByDefault(Return(std::nullopt));
    EXPECT_EQ(lister.list(query, false), (std::vector<pud_lodestar_choice>{
        choice(query, 30, pud_lodestar_side::b, pud_test_node(11), 5)}));
    EXPECT_EQ(lister.list(query, true), (std::vector<pud_lodestar_choice>{caller}));
}

TEST_F(PudLodestarChoiceListerTest, AWitnessLeafNoLongerOnTheFrontierIsLeftOut) {
    live.erase(pud_test_node(10));
    EXPECT_EQ(lister.list(query, false), (std::vector<pud_lodestar_choice>{
        choice(query, 30, pud_lodestar_side::b, pud_test_node(11), 5)}));
}

TEST_F(PudLodestarChoiceListerTest, ARefutedWitnessLeafIsLeftOut) {
    ON_CALL(refuted, is_refuted(pud_test_node(11))).WillByDefault(Return(true));
    EXPECT_EQ(lister.list(query, false), (std::vector<pud_lodestar_choice>{
        choice(query, 30, pud_lodestar_side::a, pud_test_node(10), 5)}));
}

TEST_F(PudLodestarChoiceListerTest, AWitnessLeafThatMayNotBeUnfoldedIsLeftOut) {
    ON_CALL(allower, allow(pud_test_node(10))).WillByDefault(Return(false));
    EXPECT_EQ(lister.list(query, false), (std::vector<pud_lodestar_choice>{
        choice(query, 30, pud_lodestar_side::b, pud_test_node(11), 5)}));
}

TEST_F(PudLodestarChoiceListerTest, AWitnessLeafWithoutAGoalThatHasCandidatesIsLeftOut) {
    ON_CALL(exhausted, is_exhausted(60)).WillByDefault(Return(true));
    EXPECT_THAT(lister.list(query, false), IsEmpty());
}

TEST_F(PudLodestarChoiceListerTest, AWitnessLeafIsUnfoldedOnItsSmallestGoalThatHasCandidates) {
    pud_node_queries three_goals{pud_node_queries{}.set(7, 62).set(4, 61).set(6, 63)};
    ON_CALL(node_queries, get(pud_test_node(10))).WillByDefault(ReturnRef(three_goals));
    ON_CALL(exhausted, is_exhausted(61)).WillByDefault(Return(true));
    const std::vector<pud_lodestar_choice> choices = lister.list(query, false);
    ASSERT_EQ(choices.size(), 2u);
    EXPECT_EQ(choices[0].body_goal, 6u);
}

TEST_F(PudLodestarChoiceListerTest, ACallerSlotNeedsItsQueryNodeToBeUnfoldable) {
    ON_CALL(allower, allow(pud_test_node(3))).WillByDefault(Return(false));
    EXPECT_EQ(lister.list(query, true), (std::vector<pud_lodestar_choice>{
        choice(query, 31, pud_lodestar_side::self, pud_test_node(12), 5)}));
}

TEST_F(PudLodestarChoiceListerTest, TheChoicesAreSortedBySlotWhateverTheCandidateOrder) {
    ON_CALL(candidates, candidates(40)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{31, 30}));
    ON_CALL(focus, contains(pud_test_node(11))).WillByDefault(Return(true));
    EXPECT_EQ(lister.list(query, true), (std::vector<pud_lodestar_choice>{
        choice(query, 30, pud_lodestar_side::b, pud_test_node(11), 5),
        choice(query, 31, pud_lodestar_side::self, pud_test_node(12), 5),
        caller,
    }));
}
