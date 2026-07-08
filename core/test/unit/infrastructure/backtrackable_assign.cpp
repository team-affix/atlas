// backtrackable_assign swaps a new value into a captured variable and restores the
// prior value on backtrack. Unit tests assert the forward assignment and its exact reversal.

#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include "infrastructure/backtrackable_assign.hpp"

struct BacktrackableAssignTest : public ::testing::Test {
protected:
    uint32_t x = 10;
    backtrackable_assign<uint32_t> m{99};
    void SetUp() override { m.capture(x); }
};

TEST_F(BacktrackableAssignTest, InvokeSetsNewValue) {
    m.invoke();
    EXPECT_EQ(x, 99u);
}

TEST_F(BacktrackableAssignTest, InvokeAndBacktrackRestoresValue) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(x, 10u);
}

TEST_F(BacktrackableAssignTest, RepeatedInvokeBacktrackIsStable) {
    for (int i = 0; i < 3; ++i) {
        m.invoke();
        EXPECT_EQ(x, 99u);
        m.backtrack();
        EXPECT_EQ(x, 10u);
    }
}

TEST(BacktrackableAssignNonScalar, RestoresPriorStringValue) {
    std::string s = "before";
    backtrackable_assign<std::string> m{"after"};
    m.capture(s);
    m.invoke();
    EXPECT_EQ(s, "after");
    m.backtrack();
    EXPECT_EQ(s, "before");
}
