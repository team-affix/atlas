// cumulative_grounded_weight: sim-scoped CGW register. Invariants: starts at zero,
// accumulate sums, clear resets.

#include <gtest/gtest.h>
#include "infrastructure/cumulative_grounded_weight.hpp"

struct CumulativeGroundedWeightTest : public ::testing::Test {
    cumulative_grounded_weight cgw;

    static constexpr double kWeightA = 0.25;
    static constexpr double kWeightB = 0.75;
};

TEST_F(CumulativeGroundedWeightTest, StartsAtZero) {
    EXPECT_DOUBLE_EQ(cgw.get(), 0.0);
}

TEST_F(CumulativeGroundedWeightTest, AccumulateAddsToRegister) {
    cgw.accumulate(kWeightA);
    EXPECT_DOUBLE_EQ(cgw.get(), kWeightA);
    cgw.accumulate(kWeightB);
    EXPECT_DOUBLE_EQ(cgw.get(), kWeightA + kWeightB);
}

TEST_F(CumulativeGroundedWeightTest, ClearResetsRegister) {
    cgw.accumulate(kWeightA);
    cgw.clear();
    EXPECT_DOUBLE_EQ(cgw.get(), 0.0);
}
