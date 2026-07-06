// backtrackable_deque_emplace_back appends a (move-only) element with undo
// pop_back. Unit tests assert the forward emplace, its reversal, and that
// previously appended elements keep stable addresses across the push/pop.

#include <gtest/gtest.h>
#include <deque>
#include <memory>
#include "infrastructure/backtrackable_deque_emplace_back.hpp"

using move_only_deque = std::deque<std::unique_ptr<int>>;

struct BacktrackableDequeEmplaceBackTest : public ::testing::Test {
protected:
    move_only_deque d;
    void SetUp() override { d.push_back(std::make_unique<int>(1)); }
};

TEST_F(BacktrackableDequeEmplaceBackTest, InvokeAppendsMoveOnlyValue) {
    backtrackable_deque_emplace_back<move_only_deque> m{std::make_unique<int>(2)};
    m.capture(d);
    m.invoke();
    ASSERT_EQ(d.size(), 2u);
    EXPECT_EQ(*d.back(), 2);
}

TEST_F(BacktrackableDequeEmplaceBackTest, InvokeAndBacktrackRemovesValue) {
    backtrackable_deque_emplace_back<move_only_deque> m{std::make_unique<int>(2)};
    m.capture(d);
    m.invoke();
    m.backtrack();
    ASSERT_EQ(d.size(), 1u);
    EXPECT_EQ(*d.front(), 1);
}

TEST_F(BacktrackableDequeEmplaceBackTest, ExistingElementAddressStableAcrossPush) {
    const int* addr_before = d.front().get();
    backtrackable_deque_emplace_back<move_only_deque> m{std::make_unique<int>(2)};
    m.capture(d);
    m.invoke();
    EXPECT_EQ(d.front().get(), addr_before);
}
