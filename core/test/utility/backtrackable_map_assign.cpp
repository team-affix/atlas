#include <gtest/gtest.h>
#include "../../../core/hpp/utility/backtrackable_map_assign.hpp"
#include <map>
#include <stdexcept>

class BacktrackableMapAssignTest : public ::testing::Test {
protected:
    std::map<int, int> mp{{1, 10}};
    backtrackable_map_assign<std::map<int, int>> m{1, 99};
    void SetUp() override { m.capture(mp); }
};

TEST_F(BacktrackableMapAssignTest, InvokeSetsNewValue) {
    m.invoke();
    EXPECT_EQ(mp.at(1), 99);
}

TEST_F(BacktrackableMapAssignTest, InvokeAndBacktrackRestoresOldValue) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(mp.at(1), 10);
}

TEST_F(BacktrackableMapAssignTest, InvokeWithMissingKeyThrows) {
    backtrackable_map_assign<std::map<int, int>> bad(42, 0);
    bad.capture(mp);
    EXPECT_THROW(bad.invoke(), std::out_of_range);
}
