#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_lodestar_round_robin.hpp"
#include "value_objects/pud_lodestar_choice.hpp"

namespace {

pud_lodestar_choice choice_at(uint32_t node, pud_body_goal_id goal, pud_mhcs_head_id candidate, pud_lodestar_side side) {
    return pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = pud_global_node_id{.module = 2, .node = node},
            .query_goal = goal,
            .candidate = candidate,
            .side = side,
        },
        .leaf = pud_global_node_id{.module = 1, .node = node},
        .body_goal = 0,
    };
}

struct PudLodestarRoundRobinTest : public ::testing::Test {
    pud_lodestar_round_robin round_robin;
    std::vector<pud_lodestar_choice> ring{
        choice_at(0, 0, 1, pud_lodestar_side::a),
        choice_at(0, 0, 1, pud_lodestar_side::b),
        choice_at(0, 1, 2, pud_lodestar_side::a),
    };
};

}

TEST_F(PudLodestarRoundRobinTest, AnEmptyRingDealsNothing) {
    EXPECT_EQ(round_robin.next({}), std::nullopt);
}

TEST_F(PudLodestarRoundRobinTest, TheFirstDealIsTheEarliestSlot) {
    EXPECT_EQ(round_robin.next(ring), ring[0]);
}

TEST_F(PudLodestarRoundRobinTest, EachDealIsTheSlotAfterTheLastThenItWraps) {
    EXPECT_EQ(round_robin.next(ring), ring[0]);
    EXPECT_EQ(round_robin.next(ring), ring[1]);
    EXPECT_EQ(round_robin.next(ring), ring[2]);
    EXPECT_EQ(round_robin.next(ring), ring[0]);
}

TEST_F(PudLodestarRoundRobinTest, ASingleSlotIsDealtAgainAfterTheRevolution) {
    const std::vector<pud_lodestar_choice> single{choice_at(0, 0, 4, pud_lodestar_side::b)};
    EXPECT_EQ(round_robin.next(single), single[0]);
    EXPECT_EQ(round_robin.next(single), single[0]);
}

TEST_F(PudLodestarRoundRobinTest, ARingThatLostTheLastDealtSlotContinuesAfterWhereItWas) {
    EXPECT_EQ(round_robin.next(ring), ring[0]);
    const std::vector<pud_lodestar_choice> shrunk{ring[0], ring[2]};
    EXPECT_EQ(round_robin.next(shrunk), ring[2]);
}

TEST_F(PudLodestarRoundRobinTest, ARingWithNoSlotPastTheCursorWrapsToItsFront) {
    EXPECT_EQ(round_robin.next(ring), ring[0]);
    EXPECT_EQ(round_robin.next(ring), ring[1]);
    EXPECT_EQ(round_robin.next(ring), ring[2]);
    const std::vector<pud_lodestar_choice> earlier{ring[0], ring[1]};
    EXPECT_EQ(round_robin.next(earlier), ring[0]);
}
