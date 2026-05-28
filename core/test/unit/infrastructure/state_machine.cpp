// state_machine is the coroutine wrapper used by elimination generators and iterators.
// These tests exercise suspend/resume, yield ordering, nested draining, move semantics,
// and exception propagation using coroutine-local values (no shared mutable globals).

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <vector>
#include "infrastructure/state_machine.hpp"

using ::testing::ElementsAre;

namespace {

template<typename T>
std::vector<T> collect_while_has_value(state_machine<T>& sm) {
    std::vector<T> out;
    while (!sm.done()) {
        auto v = sm.resume();
        if (v.has_value())
            out.push_back(std::move(v.value()));
    }
    return out;
}

state_machine<void> make_void_stepper() {
    co_await std::suspend_always{};
    co_return;
}

state_machine<int> make_int_two_yields() {
    co_yield 1;
    co_yield 2;
    co_await std::suspend_always{};
}

state_machine<int> make_int_five_yields() {
    constexpr int kFirst = 10;
    constexpr int kSecond = 20;
    constexpr int kThird = 30;
    constexpr int kFourth = 40;
    constexpr int kFifth = 50;
    co_yield kFirst;
    co_yield kSecond;
    co_yield kThird;
    co_yield kFourth;
    co_yield kFifth;
}

state_machine<const int*> make_pointer_yields_with_terminal_null() {
    const int a = 1;
    const int b = 2;
    const int c = 3;
    co_yield &a;
    co_yield &b;
    co_yield &c;
    co_yield nullptr;
}

state_machine<const int*> make_inner_two_yields() {
    const int x = 100;
    const int y = 200;
    co_yield &x;
    co_yield &y;
}

state_machine<const int*> make_outer_drains_inner() {
    auto inner = make_inner_two_yields();
    while (!inner.done()) {
        auto v = inner.resume();
        if (v.has_value())
            co_yield v.value();
    }
    co_yield nullptr;
}

state_machine<const int*> make_nested_three_levels() {
    auto mid = make_outer_drains_inner();
    while (!mid.done()) {
        auto v = mid.resume();
        if (v.has_value())
            co_yield v.value();
    }
}

state_machine<const int*> make_revalidation_style_nested() {
    const int conflict = 999;
    auto inner = make_inner_two_yields();
    while (!inner.done()) {
        auto elim = inner.resume();
        if (elim.has_value())
            co_yield elim.value();
    }
    co_yield &conflict;
}

state_machine<int> make_yield_then_immediate_return() {
    constexpr int kAnswer = 42;
    co_yield kAnswer;
}

state_machine<int> make_int_throws_after_first_yield() {
    co_yield 1;
    throw std::runtime_error("boom");
}

} // namespace

struct StateMachineTest : public ::testing::Test {};

TEST_F(StateMachineTest, SuspendThenCompletes) {
    auto sm = make_void_stepper();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, ResumeReturnsEachCoYield) {
    auto sm = make_int_two_yields();
    EXPECT_FALSE(sm.done());

    EXPECT_EQ(sm.resume(), 1);
    EXPECT_FALSE(sm.done());

    EXPECT_EQ(sm.resume(), 2);
    EXPECT_FALSE(sm.done());
}

// After co_yield then co_await suspend_always, resume() still returns the last
// co_yielded value and done() stays false until one more resume — stale last_yield_.
TEST_F(StateMachineTest, FinalSuspendThenReturnClearsLastYield) {
    auto sm = make_int_two_yields();
    ASSERT_EQ(sm.resume(), 1);
    ASSERT_EQ(sm.resume(), 2);
    ASSERT_FALSE(sm.done());

    auto after_suspend = sm.resume();
    EXPECT_FALSE(after_suspend.has_value());
    ASSERT_FALSE(sm.done());

    auto after_return = sm.resume();
    EXPECT_FALSE(after_return.has_value());
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, FiveYieldsCollectedInOrder) {
    auto sm = make_int_five_yields();
    EXPECT_THAT(collect_while_has_value(sm), ElementsAre(10, 20, 30, 40, 50));
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, SingleYieldThenReturnProducesOneValue) {
    auto sm = make_yield_then_immediate_return();
    EXPECT_THAT(collect_while_has_value(sm), ElementsAre(42));
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, CollectsYieldsIncludingTerminalNull) {
    auto sm = make_pointer_yields_with_terminal_null();

    auto values = collect_while_has_value(sm);
    ASSERT_EQ(values.size(), 4u);
    EXPECT_EQ(*values[0], 1);
    EXPECT_EQ(*values[1], 2);
    EXPECT_EQ(*values[2], 3);
    EXPECT_EQ(values[3], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, DrainInnerFromCallerCollectsInOrder) {
    auto inner = make_inner_two_yields();
    auto values = collect_while_has_value(inner);
    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
}

TEST_F(StateMachineTest, DrainsNestedMachineInOrder) {
    auto sm = make_outer_drains_inner();
    auto values = collect_while_has_value(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(values[2], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, NestedThreeLevelsFlattensToInnerYields) {
    auto sm = make_nested_three_levels();
    auto values = collect_while_has_value(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(values[2], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, RevalidationStyleNestedYieldsInnerThenOuter) {
    auto sm = make_revalidation_style_nested();
    auto values = collect_while_has_value(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(*values[2], 999);
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, MoveConstructCanDrainRemainingYields) {
    auto sm1 = make_int_five_yields();
    ASSERT_EQ(sm1.resume(), 10);

    state_machine<int> sm2 = std::move(sm1);
    EXPECT_THAT(collect_while_has_value(sm2), ElementsAre(20, 30, 40, 50));
    EXPECT_TRUE(sm2.done());
}

TEST_F(StateMachineTest, MoveAssignCanDrainRemainingYields) {
    auto sm1 = make_int_five_yields();
    ASSERT_EQ(sm1.resume(), 10);

    state_machine<int> sm2 = make_int_two_yields();
    sm2 = std::move(sm1);

    EXPECT_THAT(collect_while_has_value(sm2), ElementsAre(20, 30, 40, 50));
    EXPECT_TRUE(sm2.done());
}

TEST_F(StateMachineTest, DrainTwoYieldCoroutineCollectsBothValues) {
    auto sm = make_int_two_yields();
    EXPECT_THAT(collect_while_has_value(sm), ElementsAre(1, 2));
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, ExceptionPropagatesOnResume) {
    auto sm = make_int_throws_after_first_yield();
    ASSERT_EQ(sm.resume(), 1);
    EXPECT_THROW(sm.resume(), std::runtime_error);
    EXPECT_THROW(sm.resume(), std::runtime_error);
}

TEST_F(StateMachineTest, JointStyleLoopForwardsNullTerminator) {
    auto sm = make_pointer_yields_with_terminal_null();
    std::vector<const int*> forwarded;
    while (!sm.done()) {
        auto res = sm.resume();
        if (res.has_value())
            forwarded.push_back(res.value());
    }
    ASSERT_EQ(forwarded.size(), 4u);
    EXPECT_EQ(*forwarded[0], 1);
    EXPECT_EQ(*forwarded[1], 2);
    EXPECT_EQ(*forwarded[2], 3);
    EXPECT_EQ(forwarded.back(), nullptr);
}
