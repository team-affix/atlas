#include <gtest/gtest.h>
#include <memory>
#include "infrastructure/backtrackable_increment.hpp"
#include "infrastructure/trail.hpp"

struct BacktrackableIncrementIntegrationTest : public ::testing::Test {
protected:
    trail t;
    int x = 5;
};

TEST_F(BacktrackableIncrementIntegrationTest, RepeatedInvocationsEachIncrementByOne) {
    t.push();
    for (int expected = 6; expected <= 8; ++expected) {
        auto mi = std::make_unique<backtrackable_increment<int>>();
        mi->capture(x);
        mi->invoke();
        t.log(std::move(mi));
        EXPECT_EQ(x, expected);
    }
}

TEST_F(BacktrackableIncrementIntegrationTest, TrailPopReversesAllIncrements) {
    t.push();
    for (int expected = 6; expected <= 8; ++expected) {
        auto mi = std::make_unique<backtrackable_increment<int>>();
        mi->capture(x);
        mi->invoke();
        t.log(std::move(mi));
    }
    t.pop();
    EXPECT_EQ(x, 5);
}
