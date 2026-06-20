#include <gtest/gtest.h>
#include "infrastructure/tracked.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/backtrackable_increment.hpp"

struct TrackedIntegrationTest : public ::testing::Test {
protected:
    trail t;
    tracked<int, trail> v{t, 10};
};

TEST_F(TrackedIntegrationTest, MutateAndPopRevertsToPrePushValue) {
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    EXPECT_EQ(v.get(), 11);
    t.pop();
    EXPECT_EQ(v.get(), 10);
}

TEST_F(TrackedIntegrationTest, TwoMutationsInOneFrameBothRevert) {
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    EXPECT_EQ(v.get(), 12);
    t.pop();
    EXPECT_EQ(v.get(), 10);
}

TEST_F(TrackedIntegrationTest, TwoFramesInnerPopReverts) {
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    EXPECT_EQ(v.get(), 11);
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    EXPECT_EQ(v.get(), 12);
    t.pop();
    EXPECT_EQ(v.get(), 11);
    t.pop();
    EXPECT_EQ(v.get(), 10);
}
