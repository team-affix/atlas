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

coroutine<const resolution_lineage*, void> two_yield_base_pop(const resolution_lineage* a,
                                                              const resolution_lineage* b) {
    co_yield a;
    co_yield b;
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

TEST_F(DbuctHorizonFrameHubTest, PopForwardsMultipleBaseYields) {
    // The base hub can emit several armed eliminations from one unwind, so the
    // wrapper has to relay every one of them in order -- a loop that forwards only
    // the first yield, or stops early, loses refutations without any error. Two
    // yields is the smallest case that distinguishes relaying from returning once.
    resolution_lineage a{nullptr, 0};
    resolution_lineage b{nullptr, 1};

    {
        ::testing::InSequence seq;
        EXPECT_CALL(pop_cgw, pop_frame());
        EXPECT_CALL(pop_goal_weights, pop_frame());
        EXPECT_CALL(pop_base, pop_solver_frame())
            .WillOnce(::testing::Return(::testing::ByMove(two_yield_base_pop(&a, &b))));
    }

    auto sm = hub.pop_solver_frame();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), &a);
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), &b);
    while (!sm.done())
        sm.resume();
}
