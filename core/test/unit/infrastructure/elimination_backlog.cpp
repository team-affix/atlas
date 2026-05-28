// Elimination backlog: backlogged candidates keyed by parent goal and rule id.
// is_backlogged must be false after constrain removes the parent bucket.

#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/elimination_backlog.hpp"
#include "../../../core/hpp/value_objects/lineage.hpp"

struct EliminationBacklogTest : public ::testing::Test {
    elimination_backlog backlog;
    expr goal_e{expr::var{0}};
    expr head0{expr::var{1}};
    expr head1{expr::var{2}};
    rule rule0{&head0, {}};
    rule rule1{&head1, {}};
    goal_lineage parent{nullptr, 0};
    resolution_lineage rl0{&parent, 0};
    resolution_lineage rl1{&parent, 1};
};

TEST_F(EliminationBacklogTest, NotBackloggedInitially) {
    EXPECT_FALSE(backlog.is_backlogged_elimination(&rl0));
}

TEST_F(EliminationBacklogTest, InsertMarksBacklogged) {
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl0));
}

TEST_F(EliminationBacklogTest, DifferentRuleSameParentTrackedSeparately) {
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_FALSE(backlog.is_backlogged_elimination(&rl1));
    backlog.insert_backlogged_elimination(&rl1);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl1));
}

TEST_F(EliminationBacklogTest, ConstrainClearsParentBucket) {
    backlog.insert_backlogged_elimination(&rl0);
    backlog.constrain_elimination_backlog(&rl0);
    EXPECT_FALSE(backlog.is_backlogged_elimination(&rl0));
}
