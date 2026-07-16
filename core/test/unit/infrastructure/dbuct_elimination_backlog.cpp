// dbuct_elimination_backlog: insert/query and frame undo.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_elimination_backlog.hpp"
#include "value_objects/lineage.hpp"

struct DbuctEliminationBacklogTest : public ::testing::Test {
    dbuct_elimination_backlog backlog;
    goal_lineage parent{nullptr, 0};
    resolution_lineage rl0{&parent, 0};
    resolution_lineage rl1{&parent, 1};
};

TEST_F(DbuctEliminationBacklogTest, NotBackloggedInitially) {
    EXPECT_FALSE(backlog.is_backlogged_elimination(&rl0));
}

TEST_F(DbuctEliminationBacklogTest, InsertMarksBacklogged) {
    backlog.push_frame();
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl0));
}

TEST_F(DbuctEliminationBacklogTest, DuplicateInsertIsIdempotent) {
    backlog.push_frame();
    backlog.insert_backlogged_elimination(&rl0);
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl0));
}

TEST_F(DbuctEliminationBacklogTest, DifferentRulesTrackedSeparately) {
    backlog.push_frame();
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_FALSE(backlog.is_backlogged_elimination(&rl1));
    backlog.insert_backlogged_elimination(&rl1);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl1));
}

TEST_F(DbuctEliminationBacklogTest, PopFrameRevertsInsert) {
    backlog.push_frame();
    backlog.insert_backlogged_elimination(&rl0);
    backlog.pop_frame();
    EXPECT_FALSE(backlog.is_backlogged_elimination(&rl0));
}
