// dbuct_frame_bump_allocator: bump/peek and frame undo of offset.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_frame_bump_allocator.hpp"

struct DbuctFrameBumpAllocatorTest : public ::testing::Test {
    dbuct_frame_bump_allocator alloc{10};
};

TEST_F(DbuctFrameBumpAllocatorTest, PeekStartsAtInitial) {
    EXPECT_EQ(alloc.peek(), 10u);
}

TEST_F(DbuctFrameBumpAllocatorTest, BumpReturnsPriorAndAdvances) {
    EXPECT_EQ(alloc.bump(3), 10u);
    EXPECT_EQ(alloc.peek(), 13u);
    EXPECT_EQ(alloc.bump(2), 13u);
    EXPECT_EQ(alloc.peek(), 15u);
}

TEST_F(DbuctFrameBumpAllocatorTest, PopFrameRestoresOffset) {
    alloc.push_frame();
    alloc.bump(5);
    EXPECT_EQ(alloc.peek(), 15u);
    alloc.pop_frame();
    EXPECT_EQ(alloc.peek(), 10u);
}
