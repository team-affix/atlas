// backtrackable_vector_push_back appends an element with undo pop_back. Unit
// tests assert the forward push and its reversal.

#include <gtest/gtest.h>
#include <vector>
#include "infrastructure/backtrackable_vector_push_back.hpp"

struct BacktrackableVectorPushBackTest : public ::testing::Test {
protected:
    std::vector<int> v{1, 2};
    backtrackable_vector_push_back<std::vector<int>> m{3};
    void SetUp() override { m.capture(v); }
};

TEST_F(BacktrackableVectorPushBackTest, InvokeAppendsValue) {
    m.invoke();
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v.back(), 3);
}

TEST_F(BacktrackableVectorPushBackTest, InvokeAndBacktrackRestoresVector) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(v, (std::vector<int>{1, 2}));
}
