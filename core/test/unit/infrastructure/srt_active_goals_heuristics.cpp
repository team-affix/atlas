// srt_active_goals_heuristics: forwards insert/clear, default 0, set percolates max.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/srt_active_goals_heuristics.hpp"
#include "value_objects/lineage.hpp"

using ::testing::StrictMock;

namespace {

struct MockInsertActiveGoal {
    MOCK_METHOD(void, insert_active_goal, (const goal_lineage*));
};

struct MockClearActiveGoals {
    MOCK_METHOD(void, clear_active_goals, ());
};

}  // namespace

using heuristics_t = srt_active_goals_heuristics<
    MockInsertActiveGoal, MockClearActiveGoals, srt_active_goals, srt_active_goals>;

struct SrtActiveGoalsHeuristicsTest : public ::testing::Test {
    StrictMock<MockInsertActiveGoal> insert;
    StrictMock<MockClearActiveGoals> clear;
    srt_active_goals srt;
    heuristics_t heuristics{insert, clear, srt, srt};

    goal_lineage parent{nullptr, 0};
    goal_lineage child0{nullptr, 1};
    goal_lineage child1{nullptr, 2};
};

TEST_F(SrtActiveGoalsHeuristicsTest, InsertForwardsThenDefaultsLeafScoreToZero) {
    EXPECT_CALL(insert, insert_active_goal(&child0));
    heuristics.insert_active_goal(&child0);
    EXPECT_EQ(heuristics.get(&child0), 0.0);
}

TEST_F(SrtActiveGoalsHeuristicsTest, SetUpdatesLeafAndPercolatesParentMax) {
    EXPECT_CALL(insert, insert_active_goal(&parent));
    heuristics.insert_active_goal(&parent);
    srt.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();

    EXPECT_CALL(insert, insert_active_goal(&child0));
    EXPECT_CALL(insert, insert_active_goal(&child1));
    heuristics.insert_active_goal(&child0);
    heuristics.insert_active_goal(&child1);
    srt.insert_active_goal(&child0);
    srt.insert_active_goal(&child1);
    srt.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();

    heuristics.set_active_goal_value(&child0, -2.0);
    heuristics.set_active_goal_value(&child1, -5.0);
    EXPECT_EQ(heuristics.get(&child0), -2.0);
    EXPECT_EQ(heuristics.get(&child1), -5.0);
    EXPECT_EQ(heuristics.get(&parent), -2.0);
}

TEST_F(SrtActiveGoalsHeuristicsTest, PercolateEarlyExitsWhenMaxUnchanged) {
    EXPECT_CALL(insert, insert_active_goal(&parent));
    heuristics.insert_active_goal(&parent);
    srt.insert_active_goal(&parent);
    srt.flush_srt_goal_batch();

    EXPECT_CALL(insert, insert_active_goal(&child0));
    EXPECT_CALL(insert, insert_active_goal(&child1));
    heuristics.insert_active_goal(&child0);
    heuristics.insert_active_goal(&child1);
    srt.insert_active_goal(&child0);
    srt.insert_active_goal(&child1);
    srt.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();

    heuristics.set_active_goal_value(&child0, -2.0);
    heuristics.set_active_goal_value(&child1, -5.0);
    EXPECT_EQ(heuristics.get(&parent), -2.0);

    heuristics.set_active_goal_value(&child1, -4.0);
    EXPECT_EQ(heuristics.get(&parent), -2.0);
}

TEST_F(SrtActiveGoalsHeuristicsTest, ClearForwardsThenRemovesScores) {
    EXPECT_CALL(insert, insert_active_goal(&child0));
    heuristics.insert_active_goal(&child0);
    EXPECT_CALL(clear, clear_active_goals());
    heuristics.clear_active_goals();
    EXPECT_THROW(heuristics.get(&child0), std::out_of_range);
}
