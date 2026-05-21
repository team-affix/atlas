#include <gtest/gtest.h>
#include "../../../core/hpp/utility/backtrackable_set_insert.hpp"
#include <set>
#include <stdexcept>

class BacktrackableSetInsertTest : public ::testing::Test {
protected:
    std::set<int> s;
    backtrackable_set_insert<std::set<int>> m{7};
    void SetUp() override { m.capture(s); }
};

TEST_F(BacktrackableSetInsertTest, InvokeInsertsValue) {
    m.invoke();
    EXPECT_EQ(s.count(7), 1u);
}

TEST_F(BacktrackableSetInsertTest, InvokeAndBacktrackRemovesValue) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(s.count(7), 0u);
}

TEST_F(BacktrackableSetInsertTest, InvokeWithDuplicateValueThrows) {
    s.insert(7);
    EXPECT_THROW(m.invoke(), std::logic_error);
}
