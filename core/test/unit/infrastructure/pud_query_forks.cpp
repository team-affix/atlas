#include <gtest/gtest.h>
#include "infrastructure/pud_query_forks.hpp"

struct PudQueryForksTest : public ::testing::Test {
    pud_query_forks forks;
};

TEST_F(PudQueryForksTest, NothingIsForkedAtFirst) {
    EXPECT_FALSE(forks.check_query_forked(1, 40));
}

TEST_F(PudQueryForksTest, MarkedForkIsSeenOnlyAtItsNodeForItsHead) {
    forks.mark_query_forked(1, 40);
    EXPECT_TRUE(forks.check_query_forked(1, 40));
    EXPECT_FALSE(forks.check_query_forked(2, 40));
    EXPECT_FALSE(forks.check_query_forked(1, 41));
}

TEST_F(PudQueryForksTest, ANodeCanForkSeveralHeads) {
    forks.mark_query_forked(1, 40);
    forks.mark_query_forked(1, 41);
    EXPECT_TRUE(forks.check_query_forked(1, 40));
    EXPECT_TRUE(forks.check_query_forked(1, 41));
}

TEST_F(PudQueryForksTest, MarkingTwiceIsHarmless) {
    forks.mark_query_forked(1, 40);
    forks.mark_query_forked(1, 40);
    EXPECT_TRUE(forks.check_query_forked(1, 40));
}
