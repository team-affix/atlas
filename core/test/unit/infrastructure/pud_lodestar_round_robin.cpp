#include <optional>
#include <stdexcept>
#include <vector>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_round_robin.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockListChoices {
    MOCK_METHOD(std::vector<pud_lodestar_choice>, list, (const pud_leaf_goal_query&, bool), (const));
};

using test_round_robin_t = pud_lodestar_round_robin<NiceMock<MockListChoices>>;

pud_leaf_goal_query query_on(pud_node_id node, pud_body_goal_id goal) {
    return pud_leaf_goal_query{.node = pud_test_node(node), .goal = goal, .head = 40, .has_candidates = true};
}

pud_lodestar_choice choice_on(
    const pud_leaf_goal_query& query, pud_mhcs_head_id candidate, pud_lodestar_side side, pud_node_id leaf) {
    return pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = query.node, .query_goal = query.goal, .candidate = candidate, .side = side},
        .leaf = pud_test_node(leaf),
        .body_goal = 0,
    };
}

// Three queries in (leaf, goal) order. The first has two slots, the second none and the third one, so the
// ring is first_a, first_b, third.
struct PudLodestarRoundRobinTest : public ::testing::Test {
    PudLodestarRoundRobinTest();
    NiceMock<MockListChoices> lister;
    test_round_robin_t round_robin;
    pud_leaf_goal_query first;
    pud_leaf_goal_query second;
    pud_leaf_goal_query third;
    std::vector<pud_leaf_goal_query> queries;
    pud_lodestar_choice first_a;
    pud_lodestar_choice first_b;
    pud_lodestar_choice third_a;

    void SetUp() override {
        ON_CALL(lister, list(_, _)).WillByDefault(Return(std::vector<pud_lodestar_choice>{}));
        ON_CALL(lister, list(first, false)).WillByDefault(Return(std::vector<pud_lodestar_choice>{first_a, first_b}));
        ON_CALL(lister, list(third, false)).WillByDefault(Return(std::vector<pud_lodestar_choice>{third_a}));
    }
};

PudLodestarRoundRobinTest::PudLodestarRoundRobinTest()
    : lister()
    , round_robin(lister, false)
    , first(query_on(1, 0))
    , second(query_on(1, 1))
    , third(query_on(2, 0))
    , queries{first, second, third}
    , first_a(choice_on(first, 5, pud_lodestar_side::a, 10))
    , first_b(choice_on(first, 5, pud_lodestar_side::b, 11))
    , third_a(choice_on(third, 6, pud_lodestar_side::a, 12)) {}

}

TEST_F(PudLodestarRoundRobinTest, NoQueriesDealNothing) {
    EXPECT_EQ(round_robin.next({}), std::nullopt);
}

TEST_F(PudLodestarRoundRobinTest, QueriesWithoutChoicesDealNothing) {
    EXPECT_EQ(round_robin.next({second}), std::nullopt);
}

TEST_F(PudLodestarRoundRobinTest, TheFirstDealIsTheEarliestSlot) {
    EXPECT_EQ(round_robin.next(queries), first_a);
}

TEST_F(PudLodestarRoundRobinTest, EachDealIsTheSlotAfterTheLastAcrossQueriesThenItWraps) {
    EXPECT_EQ(round_robin.next(queries), first_a);
    EXPECT_EQ(round_robin.next(queries), first_b);
    EXPECT_EQ(round_robin.next(queries), third_a);
    EXPECT_EQ(round_robin.next(queries), first_a);
}

TEST_F(PudLodestarRoundRobinTest, ItListsItsOwnRing) {
    test_round_robin_t focus_round_robin{lister, true};
    EXPECT_CALL(lister, list(first, true)).WillOnce(Return(std::vector<pud_lodestar_choice>{first_b}));
    EXPECT_EQ(focus_round_robin.next(queries), first_b);
}

TEST_F(PudLodestarRoundRobinTest, TheWalkStopsAtTheFirstSlotPastTheCursor) {
    EXPECT_CALL(lister, list(_, _)).Times(AnyNumber());
    EXPECT_CALL(lister, list(second, _)).Times(0);
    EXPECT_CALL(lister, list(third, _)).Times(0);
    EXPECT_EQ(round_robin.next(queries), first_a);
}

TEST_F(PudLodestarRoundRobinTest, QueriesBeforeTheCursorAreNotListedWhileASlotLiesPastIt) {
    EXPECT_EQ(round_robin.next(queries), first_a);
    EXPECT_EQ(round_robin.next(queries), first_b);
    const pud_leaf_goal_query fourth = query_on(3, 0);
    const pud_lodestar_choice fourth_a = choice_on(fourth, 7, pud_lodestar_side::a, 13);
    ON_CALL(lister, list(fourth, false)).WillByDefault(Return(std::vector<pud_lodestar_choice>{fourth_a}));
    EXPECT_EQ(round_robin.next({first, second, third, fourth}), third_a);
    EXPECT_CALL(lister, list(_, _)).Times(AnyNumber());
    EXPECT_CALL(lister, list(first, _)).Times(0);
    EXPECT_CALL(lister, list(second, _)).Times(0);
    EXPECT_EQ(round_robin.next({first, second, third, fourth}), fourth_a);
}

TEST_F(PudLodestarRoundRobinTest, WithNoEarlierChoicesTheCursorsOwnQueryWrapsToItsFront) {
    EXPECT_EQ(round_robin.next({first}), first_a);
    EXPECT_EQ(round_robin.next({first}), first_b);
    EXPECT_EQ(round_robin.next({first}), first_a);
}

TEST_F(PudLodestarRoundRobinTest, ASingleSlotIsDealtAgainAfterTheRevolution) {
    EXPECT_EQ(round_robin.next({third}), third_a);
    EXPECT_EQ(round_robin.next({third}), third_a);
}

TEST_F(PudLodestarRoundRobinTest, ARingThatLostTheLastDealtSlotContinuesAfterWhereItWas) {
    EXPECT_EQ(round_robin.next(queries), first_a);
    ON_CALL(lister, list(first, false)).WillByDefault(Return(std::vector<pud_lodestar_choice>{first_b}));
    EXPECT_EQ(round_robin.next(queries), first_b);
    ON_CALL(lister, list(first, false)).WillByDefault(Return(std::vector<pud_lodestar_choice>{first_a}));
    EXPECT_EQ(round_robin.next(queries), third_a);
}

TEST_F(PudLodestarRoundRobinTest, ARingWhoseCursorQueryIsGoneWrapsToItsFront) {
    EXPECT_EQ(round_robin.next(queries), first_a);
    EXPECT_EQ(round_robin.next(queries), first_b);
    EXPECT_EQ(round_robin.next(queries), third_a);
    EXPECT_EQ(round_robin.next({first, second}), first_a);
}

TEST_F(PudLodestarRoundRobinTest, ChoicesSharingASlotAreDealtOnceARevolution) {
    const pud_lodestar_choice first_b_again = choice_on(first, 5, pud_lodestar_side::b, 14);
    ON_CALL(lister, list(first, false))
        .WillByDefault(Return(std::vector<pud_lodestar_choice>{first_a, first_b, first_b_again}));
    EXPECT_EQ(round_robin.next(queries), first_a);
    EXPECT_EQ(round_robin.next(queries), first_b);
    EXPECT_EQ(round_robin.next(queries), third_a);
}

TEST_F(PudLodestarRoundRobinTest, QueriesOutOfLeafAndGoalOrderBreakTheInvariant) {
    EXPECT_THROW(round_robin.next({third, first}), std::logic_error);
}
