#include <gtest/gtest.h>
#include "infrastructure/pud_refuted_leaves.hpp"

TEST(PudRefutedLeavesTest, MarkAndCheckRefuted) {
    pud_refuted_leaves refuted;
    refuted.mark_refuted(3);
    EXPECT_TRUE(refuted.check_refuted(3));
    EXPECT_FALSE(refuted.check_refuted(4));
}
