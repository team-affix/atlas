// Elimination backlog: backlogged candidates keyed by parent goal and rule id.
// Inserts are trail-backtracked like cdcl avoidances.

#include <gtest/gtest.h>
#include "infrastructure/elimination_backlog.hpp"
#include "infrastructure/trail.hpp"
#include "value_objects/lineage.hpp"

struct EliminationBacklogTest : public ::testing::Test {
    trail t;
    elimination_backlog backlog{t};
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

TEST_F(EliminationBacklogTest, PopRevertsInsert) {
    t.push();
    backlog.insert_backlogged_elimination(&rl0);
    EXPECT_TRUE(backlog.is_backlogged_elimination(&rl0));
    t.pop();
    EXPECT_FALSE(backlog.is_backlogged_elimination(&rl0));
}
