#include <optional>
#include <vector>
#include <gtest/gtest.h>
#include "infrastructure/pud_lodestar_schedule.hpp"
#include "value_objects/pud_lodestar_choice.hpp"

namespace {

pud_lodestar_choice choice_at(uint32_t node, pud_body_goal_id goal, pud_mhcs_head_id candidate, uint8_t side) {
    return pud_lodestar_choice{
        .slot = pud_lodestar_slot_id{
            .query_node = pud_global_node_id{.module = 2, .node = node},
            .query_goal = goal,
            .candidate = candidate,
            .side = side,
        },
        .leaf = pud_global_node_id{.module = 1, .node = node},
        .body_goal = 0,
        .eligible = true,
    };
}

}

TEST(PudLodestarScheduleTest, FirstPickIsTheEarliestSlot) {
    pud_lodestar_schedule schedule;
    const std::vector<pud_lodestar_choice> ring{
        choice_at(0, 0, 1, k_lodestar_side_a),
        choice_at(0, 0, 1, k_lodestar_side_b),
        choice_at(0, 1, 2, k_lodestar_side_a),
    };
    const std::optional<pud_lodestar_choice> first = schedule.next_non_conjecture(ring);
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->slot.side, k_lodestar_side_a);
    EXPECT_EQ(first->slot.candidate, 1u);
}

TEST(PudLodestarScheduleTest, RoundRobinDoesNotRepeatTheChildOfTheSameSlot) {
    pud_lodestar_schedule schedule;
    const std::vector<pud_lodestar_choice> ring{
        choice_at(0, 0, 1, k_lodestar_side_a),
        choice_at(0, 1, 2, k_lodestar_side_a),
    };
    const std::optional<pud_lodestar_choice> first = schedule.next_non_conjecture(ring);
    const std::optional<pud_lodestar_choice> second = schedule.next_non_conjecture(ring);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->slot.query_goal, 0u);
    EXPECT_EQ(second->slot.query_goal, 1u);
}

TEST(PudLodestarScheduleTest, SingleSlotIsChosenAgainAfterTheRevolution) {
    pud_lodestar_schedule schedule;
    const std::vector<pud_lodestar_choice> ring{choice_at(0, 0, 4, k_lodestar_side_b)};
    ASSERT_TRUE(schedule.next_non_conjecture(ring).has_value());
    const std::optional<pud_lodestar_choice> again = schedule.next_non_conjecture(ring);
    ASSERT_TRUE(again.has_value());
    EXPECT_EQ(again->slot.candidate, 4u);
}

TEST(PudLodestarScheduleTest, IneligibleSlotIsSkipped) {
    pud_lodestar_schedule schedule;
    std::vector<pud_lodestar_choice> ring{
        choice_at(0, 0, 1, k_lodestar_side_a),
        choice_at(0, 1, 2, k_lodestar_side_a),
    };
    ring[0].eligible = false;
    const std::optional<pud_lodestar_choice> chosen = schedule.next_non_conjecture(ring);
    ASSERT_TRUE(chosen.has_value());
    EXPECT_EQ(chosen->slot.query_goal, 1u);
}
