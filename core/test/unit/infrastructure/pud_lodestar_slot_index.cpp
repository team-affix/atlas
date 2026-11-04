#include <limits>
#include <optional>
#include <set>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_slot_index.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

struct MockApplyRegister {
    MOCK_METHOD(void, apply_register, (const pud_frontier_change&));
};
struct MockApplyUnfold {
    MOCK_METHOD(void, apply_unfold, (pud_global_node_id, const pud_frontier_change&));
};
struct MockEraseRefuted {
    MOCK_METHOD(void, erase_refuted, ((const std::vector<pud_global_node_id>&)));
};
struct MockEraseLeaf {
    MOCK_METHOD(void, erase, (pud_global_node_id));
};
struct MockTakeUnsettledLeaves {
    MOCK_METHOD(std::vector<pud_global_node_id>, take_unsettled_leaves, ());
};
struct MockTakeUnsettledSearches {
    MOCK_METHOD(std::vector<pud_mhws_head_id>, take_unsettled_searches, ());
};
struct MockGetLiveNodes {
    MOCK_METHOD(const std::set<pud_global_node_id>&, live, (), (const));
};
struct MockIsNodeRefuted {
    MOCK_METHOD(bool, is_refuted, (pud_global_node_id), (const));
};
struct MockContainsFocusNode {
    MOCK_METHOD(bool, contains, (pud_global_node_id), (const));
};
struct MockGetNodeGoals {
    MOCK_METHOD(const pud_goal_list&, get, (pud_global_node_id), (const));
};
struct MockGetQueryCandidates {
    MOCK_METHOD(std::vector<pud_mhcs_head_id>, candidates, (pud_mhq_head_id), (const));
};
struct MockFindCandidateWitnesses {
    MOCK_METHOD(const pud_candidate_witnesses*, find, (pud_mhcs_head_id), (const));
};
struct MockFindWitnessNode {
    MOCK_METHOD(std::optional<pud_global_node_id>, node_of, (pud_mhws_head_id), (const));
};
struct MockAllowUnfold {
    MOCK_METHOD(bool, allow, (pud_global_node_id), (const));
};
struct MockTakeIntroducedQueries {
    MOCK_METHOD(std::vector<pud_introduced_query>, take_introduced, ());
};
struct MockTakeWithdrawnQueries {
    MOCK_METHOD(std::vector<pud_mhq_head_id>, take_withdrawn, ());
};

using test_slot_index_t = pud_lodestar_slot_index<
    NiceMock<MockApplyRegister>, NiceMock<MockApplyUnfold>, NiceMock<MockEraseRefuted>, NiceMock<MockEraseLeaf>,
    NiceMock<MockTakeUnsettledLeaves>, NiceMock<MockTakeUnsettledSearches>, NiceMock<MockGetLiveNodes>,
    NiceMock<MockGetLiveNodes>,
    NiceMock<MockIsNodeRefuted>, NiceMock<MockContainsFocusNode>, NiceMock<MockGetNodeGoals>,
    NiceMock<MockGetQueryCandidates>, NiceMock<MockFindCandidateWitnesses>,
    NiceMock<MockFindWitnessNode>, NiceMock<MockAllowUnfold>, NiceMock<MockTakeIntroducedQueries>,
    NiceMock<MockTakeWithdrawnQueries>>;

pud_lodestar_choice choice(pud_mhcs_head_id candidate, pud_lodestar_side side, pud_node_id leaf) {
    return pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = pud_test_node(3), .query_goal = 0, .candidate = candidate, .side = side},
        .leaf = pud_test_node(leaf),
    };
}

pud_lodestar_choice caller_on(pud_node_id leaf) {
    return pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = pud_test_node(leaf),
            .query_goal = std::numeric_limits<pud_body_goal_id>::max(),
            .candidate = std::numeric_limits<pud_mhcs_head_id>::max(),
            .side = pud_lodestar_side::caller,
        },
        .leaf = pud_test_node(leaf),
    };
}

pud_frontier_change offspring(std::vector<pud_global_node_id> nodes) {
    return pud_frontier_change{.offspring = std::move(nodes), .refuted = {}};
}

// Query 40 is introduced at focus leaf 3 on its goal 0 and reported at the first settle. Candidate 30 rests on
// witness searches 50 and 51, at leaves 10 and 11 outside the focus; candidate 31 is its own witness at leaf 12,
// inside it. Leaf 3 has goal 0 open and every other leaf goal 5; query 60 on goal 5 has no candidates of its own,
// so leaf 12, a focus leaf itself, holds only its caller choice. Every leaf is live once registered, unrefuted and
// may be unfolded.
struct PudLodestarSlotIndexTest : public ::testing::Test {
    PudLodestarSlotIndexTest();
    NiceMock<MockApplyRegister> registrar;
    NiceMock<MockApplyUnfold> unfolder;
    NiceMock<MockEraseRefuted> refuted_eraser;
    NiceMock<MockEraseLeaf> eraser;
    NiceMock<MockTakeUnsettledLeaves> unsettled;
    NiceMock<MockTakeUnsettledSearches> unsettled_searches;
    NiceMock<MockGetLiveNodes> focus_frontier;
    NiceMock<MockGetLiveNodes> frontier;
    NiceMock<MockIsNodeRefuted> refuted;
    NiceMock<MockContainsFocusNode> focus;
    NiceMock<MockGetNodeGoals> node_goals;
    NiceMock<MockGetQueryCandidates> candidates;
    NiceMock<MockFindCandidateWitnesses> witness_index;
    NiceMock<MockFindWitnessNode> witnesses;
    NiceMock<MockAllowUnfold> allower;
    NiceMock<MockTakeIntroducedQueries> introductions;
    NiceMock<MockTakeWithdrawnQueries> withdrawals;
    test_slot_index_t index;
    std::set<pud_global_node_id> live_focus;
    std::set<pud_global_node_id> live;
    pud_goal_list goal_zero;
    pud_goal_list goal_five;
    pud_goal_list no_goals;
    pud_candidate_witnesses witnessed;
    pud_candidate_witnesses self_witnessed;
    pud_lodestar_choice caller;
    pud_lodestar_choice witness_caller;
    std::vector<pud_introduced_query> introduced;
    std::vector<pud_mhq_head_id> withdrawn;

    std::vector<pud_introduced_query> take_introduced() {
        return std::exchange(introduced, {});
    }

    std::vector<pud_mhq_head_id> take_withdrawn() {
        return std::exchange(withdrawn, {});
    }

    void SetUp() override {
        ON_CALL(unsettled, take_unsettled_leaves()).WillByDefault(Return(std::vector<pud_global_node_id>{}));
        ON_CALL(unsettled_searches, take_unsettled_searches()).WillByDefault(Return(std::vector<pud_mhws_head_id>{}));
        ON_CALL(focus_frontier, live()).WillByDefault(ReturnRef(live_focus));
        ON_CALL(frontier, live()).WillByDefault(ReturnRef(live));
        ON_CALL(refuted, is_refuted(_)).WillByDefault(Return(false));
        ON_CALL(focus, contains(_)).WillByDefault(Return(false));
        ON_CALL(focus, contains(pud_test_node(3))).WillByDefault(Return(true));
        ON_CALL(focus, contains(pud_test_node(12))).WillByDefault(Return(true));
        ON_CALL(node_goals, get(_)).WillByDefault(ReturnRef(goal_five));
        ON_CALL(node_goals, get(pud_test_node(3))).WillByDefault(ReturnRef(goal_zero));
        ON_CALL(candidates, candidates(_)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{}));
        ON_CALL(candidates, candidates(40)).WillByDefault(Return(std::vector<pud_mhcs_head_id>{30, 31}));
        ON_CALL(witness_index, find(30)).WillByDefault(Return(&witnessed));
        ON_CALL(witness_index, find(31)).WillByDefault(Return(&self_witnessed));
        ON_CALL(witnesses, node_of(50)).WillByDefault(Return(pud_test_node(10)));
        ON_CALL(witnesses, node_of(51)).WillByDefault(Return(pud_test_node(11)));
        ON_CALL(allower, allow(_)).WillByDefault(Return(true));
        ON_CALL(introductions, take_introduced())
            .WillByDefault(Invoke(this, &PudLodestarSlotIndexTest::take_introduced));
        ON_CALL(withdrawals, take_withdrawn()).WillByDefault(Invoke(this, &PudLodestarSlotIndexTest::take_withdrawn));
    }

    void go_live(std::vector<pud_global_node_id> nodes) {
        for (const pud_global_node_id node : nodes) {
            live.insert(node);
            if (focus.contains(node))
                live_focus.insert(node);
        }
        index.apply_register(offspring(std::move(nodes)));
    }

    void leave(pud_global_node_id node) {
        live.erase(node);
        live_focus.erase(node);
        index.erase(node);
    }

    void go_live_everywhere() {
        go_live({pud_test_node(3), pud_test_node(10), pud_test_node(11), pud_test_node(12)});
    }
};

PudLodestarSlotIndexTest::PudLodestarSlotIndexTest()
    : registrar()
    , unfolder()
    , refuted_eraser()
    , eraser()
    , unsettled()
    , unsettled_searches()
    , focus_frontier()
    , frontier()
    , refuted()
    , focus()
    , node_goals()
    , candidates()
    , witness_index()
    , witnesses()
    , allower()
    , introductions()
    , withdrawals()
    , index(
          registrar, unfolder, refuted_eraser, eraser, unsettled, unsettled_searches, focus_frontier, frontier, refuted,
          focus, node_goals, candidates, witness_index, witnesses, allower, introductions, withdrawals)
    , live_focus()
    , live()
    , goal_zero{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(0, nullptr), .next_id = 1}
    , goal_five{.goals = immer::map<pud_body_goal_id, const expr*>{}.set(5, nullptr), .next_id = 6}
    , no_goals{.goals = {}, .next_id = 6}
    , witnessed{.self_witness = false, .self_leaf = {}, .witnesses = {50, 51}}
    , self_witnessed{.self_witness = true, .self_leaf = pud_test_node(12), .witnesses = {}}
    , caller(caller_on(3))
    , witness_caller(caller_on(12))
    , introduced{pud_introduced_query{.head = 40, .node = pud_test_node(3), .goal = 0}}
    , withdrawn() {}

}

TEST_F(PudLodestarSlotIndexTest, BothRingsAreEmptyAtFirst) {
    EXPECT_THAT(index.ring(false), IsEmpty());
    EXPECT_THAT(index.ring(true), IsEmpty());
}

TEST_F(PudLodestarSlotIndexTest, EveryFrontierMutationIsForwarded) {
    const pud_frontier_change change = offspring({pud_test_node(7)});
    const std::vector<pud_global_node_id> refuted_leaves{pud_test_node(8)};
    EXPECT_CALL(registrar, apply_register(change));
    EXPECT_CALL(unfolder, apply_unfold(pud_test_node(6), change));
    EXPECT_CALL(refuted_eraser, erase_refuted(refuted_leaves));
    EXPECT_CALL(eraser, erase(pud_test_node(9)));
    index.apply_register(change);
    index.apply_unfold(pud_test_node(6), change);
    index.erase_refuted(refuted_leaves);
    index.erase(pud_test_node(9));
}

TEST_F(PudLodestarSlotIndexTest, TheOuterRingHoldsTheWitnessLeavesOutsideTheFocusBySide) {
    go_live_everywhere();
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{
        choice(30, pud_lodestar_side::a, 10),
        choice(30, pud_lodestar_side::b, 11),
    }));
}

TEST_F(PudLodestarSlotIndexTest, TheFocusRingHoldsTheWitnessLeavesInsideItAndOneCallerChoicePerLeaf) {
    go_live_everywhere();
    EXPECT_EQ(index.ring(true), (std::multiset<pud_lodestar_choice>{
        choice(31, pud_lodestar_side::self, 12),
        caller,
        witness_caller,
    }));
}

TEST_F(PudLodestarSlotIndexTest, AQueryLeafThatMayNotBeUnfoldedHoldsNoCallerChoice) {
    ON_CALL(allower, allow(pud_test_node(3))).WillByDefault(Return(false));
    go_live_everywhere();
    EXPECT_EQ(index.ring(true), (std::multiset<pud_lodestar_choice>{
        choice(31, pud_lodestar_side::self, 12),
        witness_caller,
    }));
}

TEST_F(PudLodestarSlotIndexTest, AWitnessLeafThatGoesLiveLaterGainsItsChoiceThen) {
    go_live({pud_test_node(3), pud_test_node(11)});
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{choice(30, pud_lodestar_side::b, 11)}));
    go_live({pud_test_node(10)});
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{
        choice(30, pud_lodestar_side::a, 10),
        choice(30, pud_lodestar_side::b, 11),
    }));
}

TEST_F(PudLodestarSlotIndexTest, AWitnessLeafLeavingTheFrontierTakesItsChoiceOut) {
    go_live_everywhere();
    leave(pud_test_node(10));
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{choice(30, pud_lodestar_side::b, 11)}));
}

TEST_F(PudLodestarSlotIndexTest, ARefutedWitnessLeafIsLeftOut) {
    ON_CALL(refuted, is_refuted(pud_test_node(11))).WillByDefault(Return(true));
    go_live_everywhere();
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{choice(30, pud_lodestar_side::a, 10)}));
}

TEST_F(PudLodestarSlotIndexTest, AWitnessLeafThatMayNotBeUnfoldedIsLeftOut) {
    ON_CALL(allower, allow(pud_test_node(10))).WillByDefault(Return(false));
    go_live_everywhere();
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{choice(30, pud_lodestar_side::b, 11)}));
}

TEST_F(PudLodestarSlotIndexTest, AWitnessLeafWithNoOpenGoalIsLeftOut) {
    ON_CALL(node_goals, get(pud_test_node(10))).WillByDefault(ReturnRef(no_goals));
    ON_CALL(node_goals, get(pud_test_node(11))).WillByDefault(ReturnRef(no_goals));
    go_live_everywhere();
    EXPECT_THAT(index.ring(false), IsEmpty());
}

TEST_F(PudLodestarSlotIndexTest, AnUnwatchedCandidateAddsNothing) {
    ON_CALL(witness_index, find(31)).WillByDefault(Return(nullptr));
    go_live_everywhere();
    EXPECT_EQ(index.ring(true), (std::multiset<pud_lodestar_choice>{caller, witness_caller}));
}

TEST_F(PudLodestarSlotIndexTest, AWitnessSearchThatMovedIsFollowedWhenItsOldLeafIsUnsettled) {
    go_live_everywhere();
    go_live({pud_test_node(13)});
    ON_CALL(witnesses, node_of(50)).WillByDefault(Return(pud_test_node(13)));
    EXPECT_CALL(unsettled, take_unsettled_leaves())
        .WillOnce(Return(std::vector<pud_global_node_id>{pud_test_node(10)}));
    index.erase(pud_test_node(20));
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{
        choice(30, pud_lodestar_side::a, 13),
        choice(30, pud_lodestar_side::b, 11),
    }));
}

TEST_F(PudLodestarSlotIndexTest, AFollowedWitnessSearchRestsOnItsNewLeaf) {
    go_live({pud_test_node(3), pud_test_node(11)});
    ON_CALL(witnesses, node_of(50)).WillByDefault(Return(pud_test_node(13)));
    EXPECT_CALL(unsettled, take_unsettled_leaves())
        .WillOnce(Return(std::vector<pud_global_node_id>{pud_test_node(10)}))
        .WillRepeatedly(Return(std::vector<pud_global_node_id>{}));
    index.erase(pud_test_node(20));
    go_live({pud_test_node(13)});
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{
        choice(30, pud_lodestar_side::a, 13),
        choice(30, pud_lodestar_side::b, 11),
    }));
}

TEST_F(PudLodestarSlotIndexTest, AnEndedWitnessSearchLosesItsChoice) {
    go_live_everywhere();
    ON_CALL(witnesses, node_of(51)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(unsettled, take_unsettled_leaves())
        .WillOnce(Return(std::vector<pud_global_node_id>{pud_test_node(11)}));
    index.erase(pud_test_node(20));
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{choice(30, pud_lodestar_side::a, 10)}));
}

TEST_F(PudLodestarSlotIndexTest, ALeafLeavingTheFocusFrontierTakesOnlyItsCallerChoiceOut) {
    go_live_everywhere();
    leave(pud_test_node(3));
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{
        choice(30, pud_lodestar_side::a, 10),
        choice(30, pud_lodestar_side::b, 11),
    }));
    EXPECT_EQ(index.ring(true), (std::multiset<pud_lodestar_choice>{
        choice(31, pud_lodestar_side::self, 12),
        witness_caller,
    }));
}

TEST_F(PudLodestarSlotIndexTest, ARefutedLeafHoldsNoCallerChoice) {
    ON_CALL(refuted, is_refuted(pud_test_node(3))).WillByDefault(Return(true));
    go_live_everywhere();
    EXPECT_FALSE(index.ring(true).contains(caller));
}

TEST_F(PudLodestarSlotIndexTest, AWithdrawnQueryTakesAllItsChoicesOut) {
    go_live_everywhere();
    withdrawn = {40};
    index.erase(pud_test_node(20));
    EXPECT_THAT(index.ring(false), IsEmpty());
    EXPECT_EQ(index.ring(true), (std::multiset<pud_lodestar_choice>{caller, witness_caller}));
}

TEST_F(PudLodestarSlotIndexTest, AQueryWithdrawnInTheStepItWasIntroducedOpensNothing) {
    withdrawn = {40};
    go_live_everywhere();
    EXPECT_THAT(index.ring(false), IsEmpty());
    EXPECT_EQ(index.ring(true), (std::multiset<pud_lodestar_choice>{caller, witness_caller}));
}

TEST_F(PudLodestarSlotIndexTest, AQueryIntroducedOutsideTheFocusOpensNothing) {
    introduced = {pud_introduced_query{.head = 40, .node = pud_test_node(10), .goal = 0}};
    go_live_everywhere();
    EXPECT_THAT(index.ring(false), IsEmpty());
}

TEST_F(PudLodestarSlotIndexTest, AQueryInheritedByMoreLeavesKeepsOneEntryPerWitness) {
    go_live_everywhere();
    ON_CALL(focus, contains(pud_test_node(4))).WillByDefault(Return(true));
    ON_CALL(node_goals, get(pud_test_node(4))).WillByDefault(ReturnRef(goal_zero));
    go_live({pud_test_node(4)});
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{
        choice(30, pud_lodestar_side::a, 10),
        choice(30, pud_lodestar_side::b, 11),
    }));
    EXPECT_TRUE(index.ring(true).contains(caller_on(4)));
}

TEST_F(PudLodestarSlotIndexTest, AFocusLeafWithNoOpenGoalHoldsNoCallerChoice) {
    ON_CALL(node_goals, get(pud_test_node(3))).WillByDefault(ReturnRef(no_goals));
    go_live({pud_test_node(3)});
    EXPECT_FALSE(index.ring(true).contains(caller));
}

TEST_F(PudLodestarSlotIndexTest, EqualChoicesAreKeptOncePerEntry) {
    witnessed.witnesses = {50, 51, 52};
    ON_CALL(witnesses, node_of(52)).WillByDefault(Return(pud_test_node(11)));
    go_live_everywhere();
    EXPECT_EQ(index.ring(false).count(choice(30, pud_lodestar_side::b, 11)), 2u);
    ON_CALL(witnesses, node_of(52)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(unsettled, take_unsettled_leaves())
        .WillOnce(Return(std::vector<pud_global_node_id>{pud_test_node(11)}));
    index.erase(pud_test_node(20));
    EXPECT_EQ(index.ring(false).count(choice(30, pud_lodestar_side::b, 11)), 1u);
}

TEST_F(PudLodestarSlotIndexTest, AnUnfoldSettlesItsParentAndOffspring) {
    go_live_everywhere();
    live.erase(pud_test_node(10));
    live.insert(pud_test_node(14));
    ON_CALL(witnesses, node_of(50)).WillByDefault(Return(pud_test_node(14)));
    EXPECT_CALL(unsettled, take_unsettled_leaves())
        .WillOnce(Return(std::vector<pud_global_node_id>{pud_test_node(10)}));
    index.apply_unfold(pud_test_node(10), offspring({pud_test_node(14)}));
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{
        choice(30, pud_lodestar_side::a, 14),
        choice(30, pud_lodestar_side::b, 11),
    }));
}

TEST_F(PudLodestarSlotIndexTest, ErasingRefutedLeavesSettlesThem) {
    go_live_everywhere();
    live.erase(pud_test_node(11));
    index.erase_refuted({pud_test_node(11)});
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{choice(30, pud_lodestar_side::a, 10)}));
}

TEST_F(PudLodestarSlotIndexTest, AnUnsettledLeafWhoseSearchesStayedPutReplacesNoChoice) {
    go_live_everywhere();
    EXPECT_CALL(allower, allow(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(allower, allow(pud_test_node(10))).Times(0);
    EXPECT_CALL(unsettled, take_unsettled_leaves())
        .WillOnce(Return(std::vector<pud_global_node_id>{pud_test_node(10), pud_test_node(10)}));
    index.erase(pud_test_node(20));
    EXPECT_EQ(index.ring(false).count(choice(30, pud_lodestar_side::a, 10)), 1u);
}

TEST_F(PudLodestarSlotIndexTest, AnUnsettledSearchThatEndedLosesItsChoice) {
    go_live_everywhere();
    ON_CALL(witnesses, node_of(51)).WillByDefault(Return(std::nullopt));
    EXPECT_CALL(unsettled_searches, take_unsettled_searches()).WillOnce(Return(std::vector<pud_mhws_head_id>{51}));
    index.erase(pud_test_node(20));
    EXPECT_EQ(index.ring(false), (std::multiset<pud_lodestar_choice>{choice(30, pud_lodestar_side::a, 10)}));
}

TEST_F(PudLodestarSlotIndexTest, AnUnsettledSearchNoEntryFollowsIsIgnored) {
    go_live_everywhere();
    EXPECT_CALL(witnesses, node_of(99)).Times(0);
    EXPECT_CALL(unsettled_searches, take_unsettled_searches()).WillOnce(Return(std::vector<pud_mhws_head_id>{99}));
    index.erase(pud_test_node(20));
}

TEST_F(PudLodestarSlotIndexTest, AnUnsettledSearchIsRereadAloneNotWithTheOthersOnItsLeaf) {
    witnessed.witnesses = {50, 51, 52};
    ON_CALL(witnesses, node_of(52)).WillByDefault(Return(pud_test_node(11)));
    go_live_everywhere();
    EXPECT_CALL(witnesses, node_of(_)).Times(::testing::AnyNumber());
    EXPECT_CALL(witnesses, node_of(51)).Times(0);
    EXPECT_CALL(unsettled_searches, take_unsettled_searches()).WillOnce(Return(std::vector<pud_mhws_head_id>{52}));
    index.erase(pud_test_node(20));
}
