#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/scheduler.hpp"
#include "../../../core/hpp/infrastructure/task_compare.hpp"

using ::testing::InSequence;

class MockTask : public task {
public:
    explicit MockTask(uint32_t p) : task(p) {}
    MOCK_METHOD(void, execute, (), (override));
};

TEST(TaskTest, PriorityReturnedCorrectly) {
    MockTask t(42);
    EXPECT_EQ(t.priority(), 42u);
}

TEST(TaskCompareTest, ReturnsTrueWhenFirstHasLargerPriorityValue) {
    task_compare cmp;
    MockTask a(5), b(1);
    EXPECT_TRUE(cmp(&a, &b));
}

TEST(TaskCompareTest, ReturnsFalseWhenFirstHasSmallerPriorityValue) {
    task_compare cmp;
    MockTask a(1), b(5);
    EXPECT_FALSE(cmp(&a, &b));
}

TEST(TaskCompareTest, ReturnsFalseForEqualPriorityValues) {
    task_compare cmp;
    MockTask a(3), b(3);
    EXPECT_FALSE(cmp(&a, &b));
}

TEST(SchedulerTest, TickExecutesScheduledTask) {
    scheduler s;
    MockTask t(0);
    EXPECT_CALL(t, execute());
    s.schedule(&t);
    s.tick();
}

TEST(SchedulerTest, SmallerPriorityValueExecutesFirst) {
    scheduler s;
    MockTask lo(10), hi(1);
    s.schedule(&lo);
    s.schedule(&hi);
    {
        InSequence seq;
        EXPECT_CALL(hi, execute());
        EXPECT_CALL(lo, execute());
    }
    s.tick();
    s.tick();
}

TEST(SchedulerTest, ScheduleOrderDoesNotAffectExecutionOrder) {
    scheduler s;
    MockTask lo(10), hi(1);
    s.schedule(&hi);
    s.schedule(&lo);
    {
        InSequence seq;
        EXPECT_CALL(hi, execute());
        EXPECT_CALL(lo, execute());
    }
    s.tick();
    s.tick();
}

TEST(SchedulerTest, ThreeTasksDrainSmallestPriorityValueFirst) {
    scheduler s;
    MockTask t1(3), t2(1), t3(2);
    s.schedule(&t1);
    s.schedule(&t2);
    s.schedule(&t3);
    {
        InSequence seq;
        EXPECT_CALL(t2, execute());
        EXPECT_CALL(t3, execute());
        EXPECT_CALL(t1, execute());
    }
    s.tick();
    s.tick();
    s.tick();
}

TEST(SchedulerTest, TwoTasksWithSamePriorityBothExecute) {
    scheduler s;
    MockTask t1(3), t2(3);
    EXPECT_CALL(t1, execute());
    EXPECT_CALL(t2, execute());
    s.schedule(&t1);
    s.schedule(&t2);
    s.tick();
    s.tick();
}

TEST(SchedulerTest, ScheduleAfterDrainExecutesNewTask) {
    scheduler s;
    MockTask t1(0), t2(0);
    EXPECT_CALL(t1, execute());
    s.schedule(&t1);
    s.tick();
    EXPECT_CALL(t2, execute());
    s.schedule(&t2);
    s.tick();
}

TEST(SchedulerTest, TaskScheduledBetweenTicksIsOrderedWithRemainingTasks) {
    scheduler s;
    MockTask lo(5), mid(3), hi(1);
    s.schedule(&lo);
    s.schedule(&hi);
    EXPECT_CALL(hi, execute());
    s.tick();
    s.schedule(&mid);
    {
        InSequence seq;
        EXPECT_CALL(mid, execute());
        EXPECT_CALL(lo, execute());
    }
    s.tick();
    s.tick();
}
