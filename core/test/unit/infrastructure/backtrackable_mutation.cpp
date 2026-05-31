// backtrackable_mutation is the base for trail-logged mutations: capture stores a
// reference, invoke applies the change, backtrack undoes it. These tests document
// the capture → invoke → backtrack protocol used by tracked and backtrackable_* types.

#include <gtest/gtest.h>
#include "infrastructure/backtrackable_mutation.hpp"

struct test_int_mutation : backtrackable_mutation<int> {
    void invoke() override { ref() += 1; }
    void backtrack() override { ref() -= 1; }
};

struct BacktrackableMutationTest : public ::testing::Test {
    int value = 10;
    test_int_mutation mutation;
};

TEST_F(BacktrackableMutationTest, CaptureThenInvokeMutatesTarget) {
    mutation.capture(value);
    mutation.invoke();
    EXPECT_EQ(value, 11);
}

TEST_F(BacktrackableMutationTest, BacktrackRevertsInvoke) {
    mutation.capture(value);
    mutation.invoke();
    ASSERT_EQ(value, 11);
    mutation.backtrack();
    EXPECT_EQ(value, 10);
}
