#include <gtest/gtest.h>
#include "../../../core/hpp/utility/backtrackable_increment.hpp"

struct BacktrackableIncrementTest : public ::testing::Test {
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
