// frame_bump_allocator: bump/peek and reset to initial offset.

#include <gtest/gtest.h>
#include "infrastructure/frame_bump_allocator.hpp"

struct FrameBumpAllocatorTest : public ::testing::Test {
    frame_bump_allocator alloc{10};
};

TEST_F(FrameBumpAllocatorTest, PeekStartsAtInitial) {
    EXPECT_EQ(alloc.peek(), 10u);
}

TEST_F(FrameBumpAllocatorTest, BumpReturnsPriorAndAdvances) {
    EXPECT_EQ(alloc.bump(3), 10u);
    EXPECT_EQ(alloc.peek(), 13u);
    EXPECT_EQ(alloc.bump(2), 13u);
    EXPECT_EQ(alloc.peek(), 15u);
}

TEST_F(FrameBumpAllocatorTest, ResetRestoresInitial) {
    alloc.bump(5);
    EXPECT_EQ(alloc.peek(), 15u);
    alloc.reset();
    EXPECT_EQ(alloc.peek(), 10u);
}
