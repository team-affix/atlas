// dbuct_horizon_frame_hub: delegates solver frames to a base hub and also
// push/pops goal-weight / CGW frames in mirrored order.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_horizon_frame_hub.hpp"
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

using hub_t = dbuct_horizon_frame_hub<
    MockPushSolverFrame, MockPopSolverFrame,
    MockPushFrame, MockPopFrame,
    MockPushFrame, MockPopFrame>;

struct DbuctHorizonFrameHubTest : public ::testing::Test {
    ::testing::StrictMock<MockPushSolverFrame> push_base;
    ::testing::StrictMock<MockPopSolverFrame> pop_base;
    ::testing::StrictMock<MockPushFrame> push_goal_weights;
    ::testing::StrictMock<MockPopFrame> pop_goal_weights;
    ::testing::StrictMock<MockPushFrame> push_cgw;
    ::testing::StrictMock<MockPopFrame> pop_cgw;

    hub_t hub{push_base, pop_base,
              push_goal_weights, pop_goal_weights,
              push_cgw, pop_cgw};
};

}  // namespace

TEST_F(DbuctHorizonFrameHubTest, PushThenPopUnwindsInReverseOrder) {
    {
        ::testing::InSequence seq;
        EXPECT_CALL(push_base, push_solver_frame());
        EXPECT_CALL(push_goal_weights, push_frame());
        EXPECT_CALL(push_cgw, push_frame());
    }
    hub.push_solver_frame();

    {
        ::testing::InSequence seq;
        EXPECT_CALL(pop_cgw, pop_frame());
        EXPECT_CALL(pop_goal_weights, pop_frame());
        EXPECT_CALL(pop_base, pop_solver_frame())
            .WillOnce(::testing::Return(::testing::ByMove(empty_base_pop())));
    }
    drain(hub.pop_solver_frame());
}
