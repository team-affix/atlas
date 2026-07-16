// dbuct_frontier_ready: camped frontier is already ready; activation hook is a no-op.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_frontier_ready.hpp"

TEST(DbuctFrontierReadyTest, ActivateReportsFrontierReady) {
    dbuct_frontier_ready frontier;
    EXPECT_TRUE(frontier.activate_initial_goals_and_candidates());
}
