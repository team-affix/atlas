#include <gtest/gtest.h>
#include "../../../core/hpp/utility/tracked.hpp"
#include "../../../core/hpp/utility/trail.hpp"
#include "../../../core/hpp/utility/backtrackable_increment.hpp"

class TrackedIntTest : public ::testing::Test {
protected:
    trail t;
    tracked<int> v{t, 10};
};

TEST_F(TrackedIntTest, GetReturnsInitialValue) {
    EXPECT_EQ(v.get(), 10);
}

TEST_F(TrackedIntTest, MutateChangesValue) {
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    EXPECT_EQ(v.get(), 11);
}

TEST_F(TrackedIntTest, MutateAndPopRevertsToPrePushValue) {
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    t.pop();
    EXPECT_EQ(v.get(), 10);
}

TEST_F(TrackedIntTest, TwoMutationsInOneFrameBothRevert) {
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    EXPECT_EQ(v.get(), 12);
    t.pop();
    EXPECT_EQ(v.get(), 10);
}

TEST_F(TrackedIntTest, TwoFramesInnerPopReverts) {
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    t.pop();
    EXPECT_EQ(v.get(), 11);
}

TEST_F(TrackedIntTest, TwoFramesBothPopsRevert) {
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    t.push();
    v.mutate(std::make_unique<backtrackable_increment<int>>());
    t.pop();
    t.pop();
    EXPECT_EQ(v.get(), 10);
}
