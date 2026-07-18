// srt_active_goals_heuristics: forwards insert/clear/link; set percolates max;
// link sets parent to -inf then percolates before forward.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <limits>
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

struct MockLinkSrtGoalBatchParent {
    MOCK_METHOD(void, link_srt_goal_batch_parent, (const goal_lineage*));
};

}  // namespace

using heuristics_t = srt_active_goals_heuristics<
    MockInsertActiveGoal, MockClearActiveGoals, srt_active_goals, srt_active_goals,
    MockLinkSrtGoalBatchParent>;

struct SrtActiveGoalsHeuristicsTest : public ::testing::Test {
    StrictMock<MockInsertActiveGoal> insert;
    StrictMock<MockClearActiveGoals> clear;
    StrictMock<MockLinkSrtGoalBatchParent> link;
    srt_active_goals srt;
    heuristics_t heuristics{insert, clear, srt, srt, link};

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

TEST_F(SrtActiveGoalsHeuristicsTest, LinkSetsNegInfPercolatesThenForwards) {
    goal_lineage grandparent{nullptr, 3};
    goal_lineage sibling{nullptr, 4};

    EXPECT_CALL(insert, insert_active_goal(&grandparent));
    heuristics.insert_active_goal(&grandparent);
    srt.insert_active_goal(&grandparent);
    srt.flush_srt_goal_batch();

    EXPECT_CALL(insert, insert_active_goal(&parent));
    EXPECT_CALL(insert, insert_active_goal(&sibling));
    heuristics.insert_active_goal(&parent);
    heuristics.insert_active_goal(&sibling);
    srt.insert_active_goal(&parent);
    srt.insert_active_goal(&sibling);
    srt.link_srt_goal_batch_parent(&grandparent);
    srt.flush_srt_goal_batch();

    heuristics.set_active_goal_value(&parent, -2.0);
    heuristics.set_active_goal_value(&sibling, -5.0);
    EXPECT_EQ(heuristics.get(&grandparent), -2.0);

    bool linked = false;
    EXPECT_CALL(link, link_srt_goal_batch_parent(&parent))
        .WillOnce([&](const goal_lineage*) {
            EXPECT_EQ(heuristics.get(&parent),
                      -std::numeric_limits<double>::infinity());
            EXPECT_EQ(heuristics.get(&grandparent), -5.0);
            linked = true;
        });
    heuristics.link_srt_goal_batch_parent(&parent);
    EXPECT_TRUE(linked);
    EXPECT_EQ(heuristics.get(&parent), -std::numeric_limits<double>::infinity());
    EXPECT_EQ(heuristics.get(&grandparent), -5.0);
}

TEST_F(SrtActiveGoalsHeuristicsTest, CallerSetAfterLinkOverwritesNegInfWithChildMax) {
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

    EXPECT_CALL(link, link_srt_goal_batch_parent(&parent))
        .WillOnce([&](const goal_lineage* p) {
            srt.link_srt_goal_batch_parent(p);
        });
    heuristics.link_srt_goal_batch_parent(&parent);
    srt.flush_srt_goal_batch();

    EXPECT_EQ(heuristics.get(&parent), -std::numeric_limits<double>::infinity());

    heuristics.set_active_goal_value(&child0, -2.0);
    heuristics.set_active_goal_value(&child1, -5.0);
    EXPECT_EQ(heuristics.get(&parent), -2.0);
}
