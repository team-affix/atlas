#include <gtest/gtest.h>
#include "../../../core/hpp/utility/backtrackable_increment.hpp"
#include "../../../core/hpp/utility/trail.hpp"
#include <memory>

class BacktrackableIncrementTest : public ::testing::Test {
protected:
    int x = 5;
    backtrackable_increment<int> m;
    void SetUp() override { m.capture(x); }
};

TEST_F(BacktrackableIncrementTest, InvokeIncrements) {
    m.invoke();
    EXPECT_EQ(x, 6);
}

TEST_F(BacktrackableIncrementTest, InvokeAndBacktrackDecrementsBack) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(x, 5);
}

TEST_F(BacktrackableIncrementTest, RepeatedInvocationsEachIncrementByOne) {
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

TEST_F(BacktrackableIncrementTest, TrailPopReversesAllIncrements) {
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
