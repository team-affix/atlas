#include <gtest/gtest.h>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/solver_driver.hpp"
#include "value_objects/sim_termination.hpp"

namespace {

coroutine<sim_termination, void> make_search(std::vector<sim_termination> yields) {
    for (sim_termination t : yields)
        co_yield t;
}

coroutine<sim_termination, void> make_no_yield() {
    co_return;
}

}  // namespace

struct SolverDriverTest : public ::testing::Test {
    solver_driver make_driver(std::vector<sim_termination> yields) {
        return solver_driver{make_search(std::move(yields))};
    }
};

TEST_F(SolverDriverTest, NextFalseWhenSearchDone) {
    solver_driver driver = make_driver({});
    EXPECT_FALSE(driver.next());
    EXPECT_FALSE(driver.solved());
}

TEST_F(SolverDriverTest, NextFalseWhenNoYield) {
    solver_driver driver{make_no_yield()};
    EXPECT_FALSE(driver.next());
    EXPECT_FALSE(driver.solved());
}

TEST_F(SolverDriverTest, SolvedTrueOnlyForSolvedTermination) {
    solver_driver driver = make_driver({sim_termination::solved});
    EXPECT_TRUE(driver.next());
    EXPECT_TRUE(driver.solved());
}

TEST_F(SolverDriverTest, SolvedFalseAfterConflicted) {
    solver_driver driver = make_driver({sim_termination::conflicted});
    EXPECT_TRUE(driver.next());
    EXPECT_FALSE(driver.solved());
}

TEST_F(SolverDriverTest, SolvedFalseAfterDepthExceeded) {
    solver_driver driver = make_driver({sim_termination::depth_exceeded});
    EXPECT_TRUE(driver.next());
    EXPECT_FALSE(driver.solved());
}
