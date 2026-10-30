#include <gtest/gtest.h>
#include "infrastructure/pud_lodestar_unfold_budget_store.hpp"

namespace {

struct PudLodestarUnfoldBudgetStoreTest : public ::testing::Test {
    pud_lodestar_unfold_budget_store budget;
};

}

TEST_F(PudLodestarUnfoldBudgetStoreTest, ANewStoreHasSpentNothingAndIsRunning) {
    EXPECT_EQ(budget.get_focus_unfolds(), 0u);
    EXPECT_FALSE(budget.get_stopped());
}

TEST_F(PudLodestarUnfoldBudgetStoreTest, FocusUnfoldsAndStoppedRoundTrip) {
    budget.set_focus_unfolds(4);
    budget.set_stopped(true);
    EXPECT_EQ(budget.get_focus_unfolds(), 4u);
    EXPECT_TRUE(budget.get_stopped());
    budget.set_stopped(false);
    EXPECT_FALSE(budget.get_stopped());
}
