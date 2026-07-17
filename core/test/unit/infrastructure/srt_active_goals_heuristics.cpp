// srt_active_goals_heuristics: score on insert, max-percolate on flush.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/srt_active_goals_heuristics.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockComputeActiveGoalValue {
    MOCK_METHOD(double, compute_active_goal_value, (const goal_lineage*));
};

using heuristics_t = srt_active_goals_heuristics<
    srt_active_goals, srt_active_goals, MockComputeActiveGoalValue>;

}  // namespace

struct SrtActiveGoalsHeuristicsTest : public ::testing::Test {
    srt_active_goals srt;
    NiceMock<MockComputeActiveGoalValue> compute;
    heuristics_t heuristics{srt, srt, compute};

    goal_lineage parent{nullptr, 0};
    goal_lineage child0{nullptr, 1};
    goal_lineage child1{nullptr, 2};
};

TEST_F(SrtActiveGoalsHeuristicsTest, InsertScoresLeafImmediately) {
    EXPECT_CALL(compute, compute_active_goal_value(&child0)).WillOnce(Return(-3.0));
    srt.insert_active_goal(&child0);
    heuristics.insert_active_goal(&child0);
    EXPECT_EQ(heuristics.get(&child0), -3.0);
}

TEST_F(SrtActiveGoalsHeuristicsTest, FlushDoesNotChangeRootScores) {
    EXPECT_CALL(compute, compute_active_goal_value(&child0)).WillOnce(Return(-1.0));
    EXPECT_CALL(compute, compute_active_goal_value(&child1)).WillOnce(Return(-4.0));
    srt.insert_active_goal(&child0);
    heuristics.insert_active_goal(&child0);
    srt.insert_active_goal(&child1);
    heuristics.insert_active_goal(&child1);
    srt.flush_srt_goal_batch();
    heuristics.flush_srt_goal_batch();
    EXPECT_EQ(heuristics.get(&child0), -1.0);
    EXPECT_EQ(heuristics.get(&child1), -4.0);
}

TEST_F(SrtActiveGoalsHeuristicsTest, ParentUnchangedUntilFlushThenEqualsMaxChild) {
    EXPECT_CALL(compute, compute_active_goal_value(&parent)).WillOnce(Return(-9.0));
    srt.insert_active_goal(&parent);
    heuristics.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();
    heuristics.flush_srt_goal_batch();
    EXPECT_EQ(heuristics.get(&parent), -9.0);

    EXPECT_CALL(compute, compute_active_goal_value(&child0)).WillOnce(Return(-2.0));
    EXPECT_CALL(compute, compute_active_goal_value(&child1)).WillOnce(Return(-5.0));
    srt.insert_active_goal(&child0);
    heuristics.insert_active_goal(&child0);
    srt.insert_active_goal(&child1);
    heuristics.insert_active_goal(&child1);
    srt.link_srt_goal_batch_parent(&parent);
    EXPECT_EQ(heuristics.get(&parent), -9.0);

    srt.flush_srt_goal_batch();
    heuristics.flush_srt_goal_batch();
    EXPECT_EQ(heuristics.get(&parent), -2.0);
    EXPECT_EQ(heuristics.get(&child0), -2.0);
    EXPECT_EQ(heuristics.get(&child1), -5.0);
}

TEST_F(SrtActiveGoalsHeuristicsTest, ClearRemovesScores) {
    EXPECT_CALL(compute, compute_active_goal_value(&child0)).WillOnce(Return(-1.0));
    srt.insert_active_goal(&child0);
    heuristics.insert_active_goal(&child0);
    heuristics.clear_active_goals();
    EXPECT_THROW(heuristics.get(&child0), std::out_of_range);
}
