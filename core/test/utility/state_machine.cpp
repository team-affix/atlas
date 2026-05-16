#include <gtest/gtest.h>
#include "../../../core/hpp/utility/state_machine.hpp"

namespace {

state_machine<void> make_void_stepper() {
    co_await std::suspend_always{};
    co_return;
}

state_machine<int> make_int_two_yields() {
    co_yield 1;
    co_yield 2;
    co_await std::suspend_always{};
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
