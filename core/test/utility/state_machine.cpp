#include <gtest/gtest.h>
#include "../../../core/hpp/utility/state_machine.hpp"
#include <stdexcept>

namespace {

state_machine<void> make_void_stepper() {
    co_await std::suspend_always{};
    co_return;
}

state_machine<int> make_int_generator() {
    co_yield 1;
    co_yield 2;
    co_return;
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

TEST(StateMachineInt, ResumeReturnsYieldsInOrder) {
    auto sm = make_int_generator();
    EXPECT_FALSE(sm.done());

    EXPECT_EQ(sm.resume(), 1);
    EXPECT_FALSE(sm.done());

    EXPECT_EQ(sm.resume(), 2);
    EXPECT_FALSE(sm.done());

    EXPECT_THROW({ (void)sm.resume(); }, std::logic_error);
    EXPECT_TRUE(sm.done());
}

TEST(StateMachineInt, ResumeWhenDoneThrows) {
    auto sm = make_int_generator();
    (void)sm.resume();
    (void)sm.resume();
    EXPECT_THROW({ (void)sm.resume(); }, std::logic_error);
    EXPECT_TRUE(sm.done());
    EXPECT_THROW({ (void)sm.resume(); }, std::logic_error);
}
