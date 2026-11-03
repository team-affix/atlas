#include <optional>
#include <set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/pud_lodestar_round_robin.hpp"
#include "pud_test_node.hpp"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace {

struct MockGetRing {
    MOCK_METHOD(const std::multiset<pud_lodestar_choice>&, ring, (bool), (const));
};

using test_round_robin_t = pud_lodestar_round_robin<NiceMock<MockGetRing>>;

pud_lodestar_choice choice_on(
    pud_node_id query_node, pud_mhcs_head_id candidate, pud_lodestar_side side, pud_node_id leaf) {
    return pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = pud_test_node(query_node), .query_goal = 0, .candidate = candidate, .side = side},
        .leaf = pud_test_node(leaf),
        .body_goal = 0,
    };
}

// The ring is first_a, first_b, third_a in slot order.
struct PudLodestarRoundRobinTest : public ::testing::Test {
    PudLodestarRoundRobinTest();
    NiceMock<MockGetRing> rings;
    test_round_robin_t round_robin;
    pud_lodestar_choice first_a;
    pud_lodestar_choice first_b;
    pud_lodestar_choice third_a;
    std::multiset<pud_lodestar_choice> ring;
    std::multiset<pud_lodestar_choice> other_ring;

    void SetUp() override {
        ON_CALL(rings, ring(false)).WillByDefault(ReturnRef(ring));
        ON_CALL(rings, ring(true)).WillByDefault(ReturnRef(other_ring));
    }
};

PudLodestarRoundRobinTest::PudLodestarRoundRobinTest()
    : rings()
    , round_robin(rings, false)
    , first_a(choice_on(1, 5, pud_lodestar_side::a, 10))
    , first_b(choice_on(1, 5, pud_lodestar_side::b, 11))
    , third_a(choice_on(3, 6, pud_lodestar_side::a, 12))
    , ring{third_a, first_b, first_a}
    , other_ring{} {}

}

TEST_F(PudLodestarRoundRobinTest, AnEmptyRingDealsNothing) {
    ring.clear();
    EXPECT_EQ(round_robin.next(), std::nullopt);
}

TEST_F(PudLodestarRoundRobinTest, TheFirstDealIsTheEarliestSlot) {
    EXPECT_EQ(round_robin.next(), first_a);
}

TEST_F(PudLodestarRoundRobinTest, EachDealIsTheSlotAfterTheLastThenItWraps) {
    EXPECT_EQ(round_robin.next(), first_a);
    EXPECT_EQ(round_robin.next(), first_b);
    EXPECT_EQ(round_robin.next(), third_a);
    EXPECT_EQ(round_robin.next(), first_a);
}

TEST_F(PudLodestarRoundRobinTest, ItDealsFromItsOwnRing) {
    test_round_robin_t focus_round_robin{rings, true};
    other_ring.insert(first_b);
    EXPECT_EQ(focus_round_robin.next(), first_b);
}

TEST_F(PudLodestarRoundRobinTest, ASingleSlotIsDealtAgainAfterTheRevolution) {
    ring = {third_a};
    EXPECT_EQ(round_robin.next(), third_a);
    EXPECT_EQ(round_robin.next(), third_a);
}

TEST_F(PudLodestarRoundRobinTest, ARingThatLostTheLastDealtSlotContinuesAfterWhereItWas) {
    EXPECT_EQ(round_robin.next(), first_a);
    ring.erase(first_a);
    EXPECT_EQ(round_robin.next(), first_b);
    ring.erase(first_b);
    ring.insert(first_a);
    EXPECT_EQ(round_robin.next(), third_a);
}

TEST_F(PudLodestarRoundRobinTest, ARingWithNothingPastTheCursorWrapsToItsFront) {
    EXPECT_EQ(round_robin.next(), first_a);
    EXPECT_EQ(round_robin.next(), first_b);
    EXPECT_EQ(round_robin.next(), third_a);
    ring.erase(third_a);
    EXPECT_EQ(round_robin.next(), first_a);
}

TEST_F(PudLodestarRoundRobinTest, ChoicesSharingASlotAreDealtOnceARevolution) {
    const pud_lodestar_choice first_b_again = choice_on(1, 5, pud_lodestar_side::b, 14);
    ring.insert(first_b_again);
    EXPECT_EQ(round_robin.next(), first_a);
    EXPECT_EQ(round_robin.next(), first_b);
    EXPECT_EQ(round_robin.next(), third_a);
}
