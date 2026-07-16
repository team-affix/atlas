// dbuct_cumulative_grounded_weight: accumulate with framed undo.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_cumulative_grounded_weight.hpp"

struct DbuctCumulativeGroundedWeightTest : public ::testing::Test {
    dbuct_cumulative_grounded_weight cgw;
};

TEST_F(DbuctCumulativeGroundedWeightTest, StartsAtZero) {
    EXPECT_DOUBLE_EQ(cgw.get(), 0.0);
}

TEST_F(DbuctCumulativeGroundedWeightTest, AccumulateAdds) {
    cgw.accumulate(0.25);
    cgw.accumulate(0.5);
    EXPECT_DOUBLE_EQ(cgw.get(), 0.75);
}

TEST_F(DbuctCumulativeGroundedWeightTest, PopFrameUndoesAccumulate) {
    cgw.accumulate(0.1);
    cgw.push_frame();
    cgw.accumulate(0.4);
    cgw.accumulate(0.2);
    EXPECT_NEAR(cgw.get(), 0.7, 1e-12);
    cgw.pop_frame();
    EXPECT_NEAR(cgw.get(), 0.1, 1e-12);
}
