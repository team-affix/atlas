// backtrackable_set_clear empties a set with undo that restores every element.
// Unit tests assert the forward clear and full restoration on backtrack.

#include <gtest/gtest.h>
#include <set>
#include "infrastructure/backtrackable_set_clear.hpp"

struct BacktrackableSetClearTest : public ::testing::Test {
protected:
    std::set<int> s{1, 2, 3};
    backtrackable_set_clear<std::set<int>> m;
    void SetUp() override { m.capture(s); }
};

TEST_F(BacktrackableSetClearTest, InvokeEmptiesSet) {
    m.invoke();
    EXPECT_TRUE(s.empty());
}

TEST_F(BacktrackableSetClearTest, InvokeAndBacktrackRestoresSet) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(s, (std::set<int>{1, 2, 3}));
}
