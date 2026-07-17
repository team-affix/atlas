// solve_timer: accumulates only resumed intervals; pause excludes waits.

#include <chrono>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/solve_timer.hpp"

using ::testing::Return;
using ::testing::NiceMock;

namespace {

using tp = std::chrono::steady_clock::time_point;
using dur = std::chrono::steady_clock::duration;

struct MockNow {
    using time_point = tp;
    using duration   = dur;

    MOCK_METHOD(time_point, now, (), (const));
};

tp at_ms(int64_t ms) {
    return tp{std::chrono::milliseconds{ms}};
}

}  // namespace

using test_solve_timer_t = solve_timer<MockNow>;

struct SolveTimerTest : public ::testing::Test {
    NiceMock<MockNow> clock;
    test_solve_timer_t timer{clock};
};

TEST_F(SolveTimerTest, StartsAtZeroWhilePaused) {
    EXPECT_EQ(timer.time_since_start(), dur::zero());
}

TEST_F(SolveTimerTest, AccumulatesOnlyWhileResumed) {
    EXPECT_CALL(clock, now())
        .WillOnce(Return(at_ms(1000)))   // resume
        .WillOnce(Return(at_ms(1500)))   // time_since_start while running
        .WillOnce(Return(at_ms(2000)))   // pause
        .WillOnce(Return(at_ms(5000)))   // resume after idle
        .WillOnce(Return(at_ms(5300)));  // time_since_start while running

    timer.resume();
    EXPECT_EQ(timer.time_since_start(), std::chrono::milliseconds{500});

    timer.pause();
    // Paused: no clock read; accumulated active time stays at 1s.
    EXPECT_EQ(timer.time_since_start(), std::chrono::milliseconds{1000});

    timer.resume();
    EXPECT_EQ(timer.time_since_start(), std::chrono::milliseconds{1300});
}

TEST_F(SolveTimerTest, PauseWhilePausedIsNoOp) {
    EXPECT_CALL(clock, now()).Times(0);
    timer.pause();
    EXPECT_EQ(timer.time_since_start(), dur::zero());
}

TEST_F(SolveTimerTest, ResumeWhileRunningIsNoOp) {
    EXPECT_CALL(clock, now())
        .WillOnce(Return(at_ms(0)))
        .WillOnce(Return(at_ms(100)));

    timer.resume();
    timer.resume();
    EXPECT_EQ(timer.time_since_start(), std::chrono::milliseconds{100});
}
