// dbuct_quell_frame_hub: delegates solver frames to a base hub and also
// push/pops goal-depths / work-values / remaining-work frames in mirrored order.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_quell_frame_hub.hpp"
#include "value_objects/lineage.hpp"

namespace {

struct MockPushSolverFrame {
    MOCK_METHOD(void, push_solver_frame, ());
};

struct MockPopSolverFrame {
    MOCK_METHOD((coroutine<const resolution_lineage*, void>), pop_solver_frame, ());
};

struct MockPushFrame {
    MOCK_METHOD(void, push_frame, ());
};

struct MockPopFrame {
    MOCK_METHOD(void, pop_frame, ());
};

coroutine<const resolution_lineage*, void> empty_base_pop() {
    co_return;
}

void drain(coroutine<const resolution_lineage*, void> sm) {
    while (!sm.done())
        sm.resume();
}

using hub_t = dbuct_quell_frame_hub<
    MockPushSolverFrame, MockPopSolverFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame>;

struct DbuctQuellFrameHubTest : public ::testing::Test {
    ::testing::StrictMock<MockPushSolverFrame> push_base;
    ::testing::StrictMock<MockPopSolverFrame> pop_base;
    ::testing::StrictMock<MockPushFrame> push_goal_depths;
    ::testing::StrictMock<MockPopFrame> pop_goal_depths;
    ::testing::StrictMock<MockPushFrame> push_goal_work_values;
    ::testing::StrictMock<MockPopFrame> pop_goal_work_values;
    ::testing::StrictMock<MockPushFrame> push_remaining_work;
    ::testing::StrictMock<MockPopFrame> pop_remaining_work;

    hub_t hub{push_base, pop_base,
              push_goal_depths, pop_goal_depths,
              push_goal_work_values, pop_goal_work_values,
              push_remaining_work, pop_remaining_work};
};

}  // namespace

TEST_F(DbuctQuellFrameHubTest, PushThenPopUnwindsInReverseOrder) {
    {
        ::testing::InSequence seq;
        EXPECT_CALL(push_base, push_solver_frame());
        EXPECT_CALL(push_goal_depths, push_frame());
        EXPECT_CALL(push_goal_work_values, push_frame());
        EXPECT_CALL(push_remaining_work, push_frame());
    }
    hub.push_solver_frame();

    {
        ::testing::InSequence seq;
        EXPECT_CALL(pop_remaining_work, pop_frame());
        EXPECT_CALL(pop_goal_work_values, pop_frame());
        EXPECT_CALL(pop_goal_depths, pop_frame());
        EXPECT_CALL(pop_base, pop_solver_frame())
            .WillOnce(::testing::Return(::testing::ByMove(empty_base_pop())));
    }
    drain(hub.pop_solver_frame());
}
