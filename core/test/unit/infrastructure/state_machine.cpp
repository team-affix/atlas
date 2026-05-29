// state_machine is the coroutine wrapper used by elimination generators and iterators.
// These tests exercise suspend/resume, yield ordering, nested draining, move semantics,
// and exception propagation using coroutine-local values (no shared mutable globals).

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "infrastructure/state_machine.hpp"

using ::testing::ElementsAre;

namespace {

template<typename Yield>
std::vector<Yield> collect_yields(state_machine<Yield, void>& sm) {
    std::vector<Yield> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

state_machine<void, void> make_void_stepper() {
    co_await std::suspend_always{};
    co_return;
}

state_machine<int, void> make_int_two_yields() {
    co_yield 1;
    co_yield 2;
    co_await std::suspend_always{};
}

state_machine<int, void> make_int_five_yields() {
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

state_machine<const int*, void> make_pointer_yields_with_terminal_null() {
    const int a = 1;
    const int b = 2;
    const int c = 3;
    co_yield &a;
    co_yield &b;
    co_yield &c;
    co_yield nullptr;
}

state_machine<const int*, void> make_inner_two_yields() {
    const int x = 100;
    const int y = 200;
    co_yield &x;
    co_yield &y;
}

state_machine<const int*, void> make_outer_drains_inner() {
    auto inner = make_inner_two_yields();
    while (!inner.done()) {
        inner.resume();
        if (inner.has_yield())
            co_yield inner.consume_yield();
    }
    co_yield nullptr;
}

state_machine<const int*, void> make_nested_three_levels() {
    auto mid = make_outer_drains_inner();
    while (!mid.done()) {
        mid.resume();
        if (mid.has_yield())
            co_yield mid.consume_yield();
    }
}

state_machine<const int*, void> make_revalidation_style_nested() {
    const int conflict = 999;
    auto inner = make_inner_two_yields();
    while (!inner.done()) {
        inner.resume();
        if (inner.has_yield())
            co_yield inner.consume_yield();
    }
    co_yield &conflict;
}

state_machine<int, void> make_yield_then_immediate_return() {
    constexpr int kAnswer = 42;
    co_yield kAnswer;
}

state_machine<int, void> make_int_throws_after_first_yield() {
    co_yield 1;
    throw std::runtime_error("boom");
}

state_machine<void, void> make_void_throws_on_resume() {
    co_await std::suspend_always{};
    throw std::runtime_error("void boom");
}

state_machine<void, void> make_void_suspend_twice() {
    co_await std::suspend_always{};
    co_await std::suspend_always{};
    co_return;
}

state_machine<void, int> make_suspend_then_return_int() {
    co_await std::suspend_always{};
    co_return 99;
}

state_machine<int, std::string> make_int_yield_string_return() {
    co_yield 1;
    co_yield 2;
    co_return std::string("done");
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

    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), 1);
    EXPECT_FALSE(sm.done());

    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), 2);
    EXPECT_FALSE(sm.done());
}

TEST_F(StateMachineTest, NonYieldSuspendHasNoYield) {
    auto sm = make_int_two_yields();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    sm.consume_yield();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    sm.consume_yield();
    ASSERT_FALSE(sm.done());

    sm.resume();
    EXPECT_FALSE(sm.has_yield());
    ASSERT_FALSE(sm.done());

    sm.resume();
    EXPECT_FALSE(sm.has_yield());
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, FiveYieldsCollectedInOrder) {
    auto sm = make_int_five_yields();
    EXPECT_THAT(collect_yields(sm), ElementsAre(10, 20, 30, 40, 50));
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, SingleYieldThenReturnProducesOneValue) {
    auto sm = make_yield_then_immediate_return();
    EXPECT_THAT(collect_yields(sm), ElementsAre(42));
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, CollectsYieldsIncludingTerminalNull) {
    auto sm = make_pointer_yields_with_terminal_null();

    auto values = collect_yields(sm);
    ASSERT_EQ(values.size(), 4u);
    EXPECT_EQ(*values[0], 1);
    EXPECT_EQ(*values[1], 2);
    EXPECT_EQ(*values[2], 3);
    EXPECT_EQ(values[3], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, DrainInnerFromCallerCollectsInOrder) {
    auto inner = make_inner_two_yields();
    auto values = collect_yields(inner);
    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
}

TEST_F(StateMachineTest, DrainsNestedMachineInOrder) {
    auto sm = make_outer_drains_inner();
    auto values = collect_yields(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(values[2], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, NestedThreeLevelsFlattensToInnerYields) {
    auto sm = make_nested_three_levels();
    auto values = collect_yields(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(values[2], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, RevalidationStyleNestedYieldsInnerThenOuter) {
    auto sm = make_revalidation_style_nested();
    auto values = collect_yields(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(*values[2], 999);
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, MoveConstructCanDrainRemainingYields) {
    auto sm1 = make_int_five_yields();
    sm1.resume();
    ASSERT_TRUE(sm1.has_yield());
    EXPECT_EQ(sm1.consume_yield(), 10);

    state_machine<int, void> sm2 = std::move(sm1);
    EXPECT_THAT(collect_yields(sm2), ElementsAre(20, 30, 40, 50));
    EXPECT_TRUE(sm2.done());
}

TEST_F(StateMachineTest, MoveAssignCanDrainRemainingYields) {
    auto sm1 = make_int_five_yields();
    sm1.resume();
    ASSERT_TRUE(sm1.has_yield());
    EXPECT_EQ(sm1.consume_yield(), 10);

    state_machine<int, void> sm2 = make_int_two_yields();
    sm2 = std::move(sm1);

    EXPECT_THAT(collect_yields(sm2), ElementsAre(20, 30, 40, 50));
    EXPECT_TRUE(sm2.done());
}

TEST_F(StateMachineTest, DrainTwoYieldCoroutineCollectsBothValues) {
    auto sm = make_int_two_yields();
    EXPECT_THAT(collect_yields(sm), ElementsAre(1, 2));
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, ExceptionPropagatesOnResume) {
    auto sm = make_int_throws_after_first_yield();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), 1);
    EXPECT_THROW(sm.resume(), std::runtime_error);
    EXPECT_THROW(sm.resume(), std::runtime_error);
}

TEST_F(StateMachineTest, VoidMachineExceptionPropagatesOnResume) {
    auto sm = make_void_throws_on_resume();
    sm.resume();
    EXPECT_THROW(sm.resume(), std::runtime_error);
    EXPECT_THROW(sm.resume(), std::runtime_error);
}

TEST_F(StateMachineTest, JointStyleLoopForwardsNullTerminator) {
    auto sm = make_pointer_yields_with_terminal_null();
    std::vector<const int*> forwarded;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            forwarded.push_back(sm.consume_yield());
    }
    ASSERT_EQ(forwarded.size(), 4u);
    EXPECT_EQ(*forwarded[0], 1);
    EXPECT_EQ(*forwarded[1], 2);
    EXPECT_EQ(*forwarded[2], 3);
    EXPECT_EQ(forwarded.back(), nullptr);
}

TEST_F(StateMachineTest, VoidSuspendTwiceThenReturn) {
    auto sm = make_void_suspend_twice();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(StateMachineTest, SuspendThenReturnsInt) {
    auto sm = make_suspend_then_return_int();
    sm.resume();
    EXPECT_FALSE(sm.done());
    sm.resume();
    ASSERT_TRUE(sm.done());
    EXPECT_EQ(sm.result(), 99);
}

TEST_F(StateMachineTest, ValueYieldDifferentReturnType) {
    auto sm = make_int_yield_string_return();
    std::vector<int> yields;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            yields.push_back(sm.consume_yield());
    }
    EXPECT_THAT(yields, ElementsAre(1, 2));
    ASSERT_TRUE(sm.done());
    EXPECT_EQ(sm.result(), "done");
}

TEST_F(StateMachineTest, ConsumeYieldWithoutYieldThrows) {
    auto sm = make_int_two_yields();
    EXPECT_THROW(sm.consume_yield(), std::logic_error);
}

TEST_F(StateMachineTest, ResultBeforeDoneThrows) {
    auto sm = make_suspend_then_return_int();
    EXPECT_THROW(sm.result(), std::logic_error);
}
