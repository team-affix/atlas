#include <gtest/gtest.h>
#include "../../../core/hpp/utility/backtrackable_set_erase.hpp"
#include <set>
#include <stdexcept>

class BacktrackableSetEraseTest : public ::testing::Test {
protected:
    std::set<int> s{42};
    backtrackable_set_erase<std::set<int>> m{42};
    void SetUp() override { m.capture(s); }
};

TEST_F(BacktrackableSetEraseTest, InvokeErasesValue) {
    m.invoke();
    EXPECT_EQ(s.count(42), 0u);
}

TEST_F(BacktrackableSetEraseTest, InvokeAndBacktrackReInsertsValue) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(s.count(42), 1u);
}

TEST_F(BacktrackableSetEraseTest, InvokeWithMissingValueThrows) {
    s.erase(42);
    EXPECT_THROW(m.invoke(), std::logic_error);
}
