#include <gtest/gtest.h>
#include "../../../core/hpp/utility/backtrackable_increment.hpp"
#include "../../../core/hpp/utility/trail.hpp"
#include <memory>

struct BacktrackableIncrementIntegrationTest : public ::testing::Test {
protected:
    int x = 5;
};

TEST_F(BacktrackableIncrementIntegrationTest, RepeatedInvocationsEachIncrementByOne) {
    trail t;
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
    trail t;
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
