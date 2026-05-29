// coroutine is the coroutine wrapper used by elimination generators and iterators.
// These tests exercise suspend/resume, yield ordering, nested draining, move semantics,
// and exception propagation using coroutine-local values (no shared mutable globals).

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "infrastructure/coroutine.hpp"

using ::testing::ElementsAre;

namespace {

template<typename Yield>
std::vector<Yield> collect_yields(coroutine<Yield, void>& sm) {
    std::vector<Yield> out;
    while (!sm.done()) {
        sm.resume();
        if (sm.has_yield())
            out.push_back(sm.consume_yield());
    }
    return out;
}

coroutine<void, void> make_void_stepper() {
    co_await std::suspend_always{};
    co_return;
}

coroutine<int, void> make_int_two_yields() {
    co_yield 1;
    co_yield 2;
    co_await std::suspend_always{};
}

coroutine<int, void> make_int_five_yields() {
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

coroutine<const int*, void> make_pointer_yields_with_terminal_null() {
    const int a = 1;
    const int b = 2;
    const int c = 3;
    co_yield &a;
    co_yield &b;
    co_yield &c;
    co_yield nullptr;
}

coroutine<const int*, void> make_inner_two_yields() {
    const int x = 100;
    const int y = 200;
    co_yield &x;
    co_yield &y;
}

coroutine<const int*, void> make_outer_drains_inner() {
    auto inner = make_inner_two_yields();
    while (!inner.done()) {
        inner.resume();
        if (inner.has_yield())
            co_yield inner.consume_yield();
    }
    co_yield nullptr;
}

coroutine<const int*, void> make_nested_three_levels() {
    auto mid = make_outer_drains_inner();
    while (!mid.done()) {
        mid.resume();
        if (mid.has_yield())
            co_yield mid.consume_yield();
    }
}

coroutine<const int*, void> make_revalidation_style_nested() {
    const int conflict = 999;
    auto inner = make_inner_two_yields();
    while (!inner.done()) {
        inner.resume();
        if (inner.has_yield())
            co_yield inner.consume_yield();
    }
    co_yield &conflict;
}

coroutine<int, void> make_yield_then_immediate_return() {
    constexpr int kAnswer = 42;
    co_yield kAnswer;
}

coroutine<int, void> make_int_throws_after_first_yield() {
    co_yield 1;
    throw std::runtime_error("boom");
}

coroutine<void, void> make_void_throws_on_resume() {
    co_await std::suspend_always{};
    throw std::runtime_error("void boom");
}

coroutine<void, void> make_void_suspend_twice() {
    co_await std::suspend_always{};
    co_await std::suspend_always{};
    co_return;
}

coroutine<void, int> make_suspend_then_return_int() {
    co_await std::suspend_always{};
    co_return 99;
}

coroutine<int, std::string> make_int_yield_string_return() {
    co_yield 1;
    co_yield 2;
    co_return std::string("done");
}

coroutine<int, void> yield_lvalue_int() {
    int n = 17;
    co_yield n;
}

coroutine<int, void> yield_rvalue_int() {
    co_yield 42;
}

coroutine<std::string, void> yield_lvalue_string(std::string& donor) {
    co_yield donor;
}

coroutine<std::string, void> yield_rvalue_string() {
    co_yield std::string("rvalue_yield");
}

coroutine<std::string, void> yield_moved_lvalue_string(std::string& donor) {
    co_yield std::move(donor);
}

coroutine<void, std::string> return_lvalue_string(std::string& donor) {
    co_return donor;
}

coroutine<void, std::string> return_rvalue_string() {
    co_return std::string("rvalue_return");
}

coroutine<void, std::string> return_moved_lvalue_string(std::string& donor) {
    co_return std::move(donor);
}

coroutine<std::string, std::string> yield_and_return_lvalues(std::string& yield_donor, std::string& return_donor) {
    co_yield yield_donor;
    co_return return_donor;
}

coroutine<std::string, std::string> yield_and_return_rvalues() {
    co_yield std::string("yield_rv");
    co_return std::string("return_rv");
}

coroutine<const int*, void> yield_pointer_to_caller_storage(const int* p) {
    co_yield p;
}

coroutine<int*, void> yield_pointer_to_frame_local() {
    int frame_local = 55;
    co_yield &frame_local;
    EXPECT_EQ(frame_local, 99);
}

} // namespace

struct CoroutineTest : public ::testing::Test {};

TEST_F(CoroutineTest, SuspendThenCompletes) {
    auto sm = make_void_stepper();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, ResumeReturnsEachCoYield) {
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

TEST_F(CoroutineTest, NonYieldSuspendHasNoYield) {
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

TEST_F(CoroutineTest, FiveYieldsCollectedInOrder) {
    auto sm = make_int_five_yields();
    EXPECT_THAT(collect_yields(sm), ElementsAre(10, 20, 30, 40, 50));
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, SingleYieldThenReturnProducesOneValue) {
    auto sm = make_yield_then_immediate_return();
    EXPECT_THAT(collect_yields(sm), ElementsAre(42));
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, CollectsYieldsIncludingTerminalNull) {
    auto sm = make_pointer_yields_with_terminal_null();

    auto values = collect_yields(sm);
    ASSERT_EQ(values.size(), 4u);
    EXPECT_EQ(*values[0], 1);
    EXPECT_EQ(*values[1], 2);
    EXPECT_EQ(*values[2], 3);
    EXPECT_EQ(values[3], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, DrainInnerFromCallerCollectsInOrder) {
    auto inner = make_inner_two_yields();
    auto values = collect_yields(inner);
    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
}

TEST_F(CoroutineTest, DrainsNestedMachineInOrder) {
    auto sm = make_outer_drains_inner();
    auto values = collect_yields(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(values[2], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, NestedThreeLevelsFlattensToInnerYields) {
    auto sm = make_nested_three_levels();
    auto values = collect_yields(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(values[2], nullptr);
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, RevalidationStyleNestedYieldsInnerThenOuter) {
    auto sm = make_revalidation_style_nested();
    auto values = collect_yields(sm);
    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(*values[0], 100);
    EXPECT_EQ(*values[1], 200);
    EXPECT_EQ(*values[2], 999);
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, MoveConstructCanDrainRemainingYields) {
    auto sm1 = make_int_five_yields();
    sm1.resume();
    ASSERT_TRUE(sm1.has_yield());
    EXPECT_EQ(sm1.consume_yield(), 10);

    coroutine<int, void> sm2 = std::move(sm1);
    EXPECT_THAT(collect_yields(sm2), ElementsAre(20, 30, 40, 50));
    EXPECT_TRUE(sm2.done());
}

TEST_F(CoroutineTest, MoveAssignCanDrainRemainingYields) {
    auto sm1 = make_int_five_yields();
    sm1.resume();
    ASSERT_TRUE(sm1.has_yield());
    EXPECT_EQ(sm1.consume_yield(), 10);

    coroutine<int, void> sm2 = make_int_two_yields();
    sm2 = std::move(sm1);

    EXPECT_THAT(collect_yields(sm2), ElementsAre(20, 30, 40, 50));
    EXPECT_TRUE(sm2.done());
}

TEST_F(CoroutineTest, DrainTwoYieldCoroutineCollectsBothValues) {
    auto sm = make_int_two_yields();
    EXPECT_THAT(collect_yields(sm), ElementsAre(1, 2));
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, ExceptionPropagatesOnResume) {
    auto sm = make_int_throws_after_first_yield();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), 1);
    EXPECT_THROW(sm.resume(), std::runtime_error);
    EXPECT_THROW(sm.resume(), std::runtime_error);
}

TEST_F(CoroutineTest, VoidMachineExceptionPropagatesOnResume) {
    auto sm = make_void_throws_on_resume();
    sm.resume();
    EXPECT_THROW(sm.resume(), std::runtime_error);
    EXPECT_THROW(sm.resume(), std::runtime_error);
}

TEST_F(CoroutineTest, JointStyleLoopForwardsNullTerminator) {
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

TEST_F(CoroutineTest, VoidSuspendTwiceThenReturn) {
    auto sm = make_void_suspend_twice();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_FALSE(sm.done());
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, SuspendThenReturnsInt) {
    auto sm = make_suspend_then_return_int();
    sm.resume();
    EXPECT_FALSE(sm.done());
    sm.resume();
    ASSERT_TRUE(sm.done());
    EXPECT_EQ(sm.result(), 99);
}

TEST_F(CoroutineTest, ValueYieldDifferentReturnType) {
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

TEST_F(CoroutineTest, ConsumeYieldWithoutYieldThrows) {
    auto sm = make_int_two_yields();
    EXPECT_THROW(sm.consume_yield(), std::logic_error);
}

TEST_F(CoroutineTest, ResultBeforeDoneThrows) {
    auto sm = make_suspend_then_return_int();
    EXPECT_THROW(sm.result(), std::logic_error);
}

TEST_F(CoroutineTest, YieldLvalueInt) {
    auto sm = yield_lvalue_int();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), 17);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, YieldRvalueInt) {
    auto sm = yield_rvalue_int();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), 42);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, YieldLvalueStringCopiesDonor) {
    std::string donor = "lvalue_yield";
    auto sm = yield_lvalue_string(donor);
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(donor, "lvalue_yield");
    std::string consumed = sm.consume_yield();
    EXPECT_EQ(consumed, "lvalue_yield");
    consumed = "mutated";
    EXPECT_EQ(donor, "lvalue_yield");
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, YieldRvalueString) {
    auto sm = yield_rvalue_string();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), "rvalue_yield");
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, YieldMovedLvalueEmptiesDonorAndReturnsCopy) {
    std::string donor = "moved_yield";
    auto sm = yield_moved_lvalue_string(donor);
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_TRUE(donor.empty());
    std::string consumed = sm.consume_yield();
    EXPECT_EQ(consumed, "moved_yield");
    consumed = "mutated";
    EXPECT_TRUE(donor.empty());
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, ReturnLvalueStringCopiesDonor) {
    std::string donor = "lvalue_return";
    auto sm = return_lvalue_string(donor);
    sm.resume();
    EXPECT_EQ(donor, "lvalue_return");
    sm.resume();
    ASSERT_TRUE(sm.done());
    EXPECT_EQ(sm.result(), "lvalue_return");
}

TEST_F(CoroutineTest, ReturnRvalueString) {
    auto sm = return_rvalue_string();
    sm.resume();
    sm.resume();
    ASSERT_TRUE(sm.done());
    EXPECT_EQ(sm.result(), "rvalue_return");
}

TEST_F(CoroutineTest, ReturnMovedLvalueEmptiesDonor) {
    std::string donor = "moved_return";
    auto sm = return_moved_lvalue_string(donor);
    sm.resume();
    EXPECT_TRUE(donor.empty());
    sm.resume();
    ASSERT_TRUE(sm.done());
    EXPECT_EQ(sm.result(), "moved_return");
}

TEST_F(CoroutineTest, YieldLvalueAndReturnLvalueString) {
    std::string yield_donor = "yield_lv";
    std::string return_donor = "return_lv";
    auto sm = yield_and_return_lvalues(yield_donor, return_donor);
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(yield_donor, "yield_lv");
    EXPECT_EQ(sm.consume_yield(), "yield_lv");
    sm.resume();
    ASSERT_TRUE(sm.done());
    EXPECT_EQ(return_donor, "return_lv");
    EXPECT_EQ(sm.result(), "return_lv");
}

TEST_F(CoroutineTest, YieldPointerToCallerStorageStaysValidWhileSuspended) {
    int caller_storage = 77;
    auto sm = yield_pointer_to_caller_storage(&caller_storage);
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    const int* p = sm.consume_yield();
    ASSERT_EQ(p, &caller_storage);
    EXPECT_EQ(*p, 77);
    caller_storage = 88;
    EXPECT_EQ(*p, 88);
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, YieldPointerToFrameLocalValidUntilResumeContinues) {
    auto sm = yield_pointer_to_frame_local();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    int* p = sm.consume_yield();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 55);
    *p = 99;
    sm.resume();
    EXPECT_TRUE(sm.done());
}

TEST_F(CoroutineTest, YieldRvalueAndReturnRvalueString) {
    auto sm = yield_and_return_rvalues();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), "yield_rv");
    sm.resume();
    ASSERT_TRUE(sm.done());
    EXPECT_EQ(sm.result(), "return_rv");
}
