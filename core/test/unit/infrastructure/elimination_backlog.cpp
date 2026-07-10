// Elimination backlog: backlogged candidates keyed by parent goal and rule id.
// Inserts are frame-journaled; pop_frame reverts mutations in the current frame.

#include <gtest/gtest.h>
#include "infrastructure/elimination_backlog.hpp"
#include "value_objects/lineage.hpp"

struct EliminationBacklogDuplicateInsertTest : public ::testing::Test {
    elimination_backlog backlog;
    goal_lineage parent{nullptr, 0};
    resolution_lineage rl0{&parent, 0};
};

TEST_F(EliminationBacklogDuplicateInsertTest, DuplicateInsertIsIdempotent) {
    backlog.push_frame();
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl0));
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl0));
}

struct EliminationBacklogTest : public ::testing::Test {
    elimination_backlog backlog;
    goal_lineage parent{nullptr, 0};
    resolution_lineage rl0{&parent, 0};
    resolution_lineage rl1{&parent, 1};
};

TEST_F(EliminationBacklogTest, NotBackloggedInitially) {
    EXPECT_FALSE(backlog.is_backlogged_elimination(&rl0));
}

TEST_F(EliminationBacklogTest, InsertMarksBacklogged) {
    backlog.push_frame();
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl0));
}

TEST_F(EliminationBacklogTest, DifferentRuleSameParentTrackedSeparately) {
    backlog.push_frame();
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_FALSE(backlog.is_backlogged_elimination(&rl1));
    backlog.insert_backlogged_elimination(&rl1);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl1));
}

TEST_F(EliminationBacklogTest, PopRevertsInsert) {
    backlog.push_frame();
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl0));
    backlog.pop_frame();
    EXPECT_FALSE(backlog.is_backlogged_elimination(&rl0));
}

TEST_F(EliminationBacklogTest, PushPopTracksDepth) {
    EXPECT_EQ(backlog.depth(), 0u);
    backlog.push_frame();
    EXPECT_EQ(backlog.depth(), 1u);
    backlog.pop_frame();
    EXPECT_EQ(backlog.depth(), 0u);
}
