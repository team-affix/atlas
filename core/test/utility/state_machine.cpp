#include <gtest/gtest.h>
#include <vector>
#include "../../../core/hpp/utility/state_machine.hpp"

namespace {

int sm_a = 1;
int sm_b = 2;
int sm_c = 3;
int sm_x = 100;
int sm_y = 200;
int sm_conflict = 999;

// Collects values produced the same way as elimination generators in the codebase:
//   while (!sm.done()) {
//       auto v = sm.resume();
//       if (v.has_value())
//           ... use v.value() ...
//   }
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
    co_yield 10;
    co_yield 20;
    co_yield 30;
    co_yield 40;
    co_yield 50;
}

state_machine<const int*> make_pointer_yields_with_terminal_null() {
    co_yield &sm_a;
    co_yield &sm_b;
    co_yield &sm_c;
    co_yield nullptr;
}

state_machine<const int*> make_inner_two_yields() {
    co_yield &sm_x;
    co_yield &sm_y;
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
    auto inner = make_inner_two_yields();
    while (!inner.done()) {
        auto elim = inner.resume();
        if (elim.has_value())
            co_yield elim.value();
    }
    co_yield &sm_conflict;
}

state_machine<int> make_yield_then_immediate_return() {
    co_yield 42;
}

} // namespace

TEST(StateMachineVoid, SuspendThenCompletes) {
    auto sm = make_void_stepper();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST(StateMachineInt, ResumeReturnsEachCoYield) {
    auto sm = make_int_two_yields();
    EXPECT_FALSE(sm.done());

    EXPECT_EQ(sm.resume(), 1);
    EXPECT_FALSE(sm.done());

    EXPECT_EQ(sm.resume(), 2);
    EXPECT_FALSE(sm.done());
}

// After co_yield then co_await suspend_always, resume() still returns the last
// co_yielded value and done() stays false until one more resume — stale last_yield_.
TEST(StateMachineInt, FinalSuspendThenReturnClearsLastYield) {
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

TEST(StateMachineInt, FiveYieldsCollectedInOrder) {
    auto sm = make_int_five_yields();
    EXPECT_EQ(collect_while_has_value(sm), (std::vector<int>{10, 20, 30, 40, 50}));
    EXPECT_TRUE(sm.done());
}

TEST(StateMachineInt, SingleYieldThenReturnProducesOneValue) {
    auto sm = make_yield_then_immediate_return();
    auto values = collect_while_has_value(sm);
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(values[0], 42);
    EXPECT_TRUE(sm.done());
}

TEST(StateMachinePointer, CollectsYieldsIncludingTerminalNull) {
    auto sm = make_pointer_yields_with_terminal_null();

    auto values = collect_while_has_value(sm);
    ASSERT_EQ(values.size(), 4u);
    EXPECT_EQ(*values[0], 1);
    EXPECT_EQ(*values[1], 2);
    EXPECT_EQ(*values[2], 3);
    EXPECT_EQ(values[3], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST(StateMachinePointer, DrainInnerFromCallerCollectsInOrder) {
    auto inner = make_inner_two_yields();
    auto values = collect_while_has_value(inner);
    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
}

TEST(StateMachinePointer, DrainsNestedMachineInOrder) {
    auto sm = make_outer_drains_inner();
    auto values = collect_while_has_value(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(values[2], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST(StateMachinePointer, NestedThreeLevelsFlattensToInnerYields) {
    auto sm = make_nested_three_levels();
    auto values = collect_while_has_value(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(values[2], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST(StateMachinePointer, RevalidationStyleNestedYieldsInnerThenOuter) {
    auto sm = make_revalidation_style_nested();
    auto values = collect_while_has_value(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(*values[2], 999);
    EXPECT_TRUE(sm.done());
}

TEST(StateMachineInt, MoveConstructCanDrainRemainingYields) {
    auto sm1 = make_int_five_yields();
    ASSERT_EQ(sm1.resume(), 10);

    state_machine<int> sm2 = std::move(sm1);
    auto rest = collect_while_has_value(sm2);
    EXPECT_EQ(rest, (std::vector<int>{20, 30, 40, 50}));
    EXPECT_TRUE(sm2.done());
}

TEST(StateMachineInt, MoveAssignCanDrainRemainingYields) {
    auto sm1 = make_int_five_yields();
    ASSERT_EQ(sm1.resume(), 10);

    state_machine<int> sm2 = make_int_two_yields();
    sm2 = std::move(sm1);

    auto rest = collect_while_has_value(sm2);
    EXPECT_EQ(rest, (std::vector<int>{20, 30, 40, 50}));
    EXPECT_TRUE(sm2.done());
}

TEST(StateMachineInt, DrainTwoYieldCoroutineCollectsBothValues) {
    auto sm = make_int_two_yields();
    auto values = collect_while_has_value(sm);
    EXPECT_EQ(values, (std::vector<int>{1, 2}));
    EXPECT_TRUE(sm.done());
}

TEST(StateMachinePointer, JointStyleLoopForwardsNullTerminator) {
    auto sm = make_pointer_yields_with_terminal_null();
    std::vector<const int*> forwarded;
    while (!sm.done()) {
        auto res = sm.resume();
        if (res.has_value())
            forwarded.push_back(res.value());
    }
    ASSERT_EQ(forwarded.size(), 4u);
    EXPECT_EQ(forwarded.back(), nullptr);
}
