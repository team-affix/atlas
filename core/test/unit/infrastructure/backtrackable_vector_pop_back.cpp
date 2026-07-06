// backtrackable_vector_pop_back removes the back element with undo push_back.
// Unit tests assert the forward pop and that backtrack restores the exact value.

#include <gtest/gtest.h>
#include <vector>
#include "infrastructure/backtrackable_vector_pop_back.hpp"

struct BacktrackableVectorPopBackTest : public ::testing::Test {
protected:
    std::vector<int> v{1, 2, 3};
    backtrackable_vector_pop_back<std::vector<int>> m;
    void SetUp() override { m.capture(v); }
};

TEST_F(BacktrackableVectorPopBackTest, InvokeRemovesBack) {
    m.invoke();
    EXPECT_EQ(v, (std::vector<int>{1, 2}));
}

TEST_F(BacktrackableVectorPopBackTest, InvokeAndBacktrackRestoresValue) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
}
