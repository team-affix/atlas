// dbuct_rp_heuristic_rollout_frame_hub: push/pop order + yield forwarding.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/coroutine.hpp"
#include "infrastructure/dbuct_rp_heuristic_rollout_frame_hub.hpp"
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

coroutine<const resolution_lineage*, void> one_yield_base_pop(const resolution_lineage* rl) {
    co_yield rl;
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

using hub_t = dbuct_rp_heuristic_rollout_frame_hub<
    MockPushSolverFrame, MockPopSolverFrame, MockPushFrame, MockPopFrame>;

struct DbuctRpHeuristicRolloutFrameHubTest : public ::testing::Test {
    ::testing::StrictMock<MockPushSolverFrame> push_base;
    ::testing::StrictMock<MockPopSolverFrame> pop_base;
    ::testing::StrictMock<MockPushFrame> push_map;
    ::testing::StrictMock<MockPopFrame> pop_map;

    hub_t hub{push_base, pop_base, push_map, pop_map};
};

}  // namespace

TEST_F(DbuctRpHeuristicRolloutFrameHubTest, PushThenPopUnwindsInReverseOrder) {
    {
        ::testing::InSequence seq;
        EXPECT_CALL(push_base, push_solver_frame());
        EXPECT_CALL(push_map, push_frame());
    }
    hub.push_solver_frame();

    {
        ::testing::InSequence seq;
        EXPECT_CALL(pop_map, pop_frame());
        EXPECT_CALL(pop_base, pop_solver_frame())
            .WillOnce(::testing::Return(::testing::ByMove(empty_base_pop())));
    }
    drain(hub.pop_solver_frame());
}

TEST_F(DbuctRpHeuristicRolloutFrameHubTest, PopForwardsBaseYields) {
    resolution_lineage rl{nullptr, 0};

    EXPECT_CALL(pop_map, pop_frame());
    EXPECT_CALL(pop_base, pop_solver_frame())
        .WillOnce(::testing::Return(::testing::ByMove(one_yield_base_pop(&rl))));

    auto sm = hub.pop_solver_frame();
    ASSERT_FALSE(sm.done());
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), &rl);
    while (!sm.done())
        sm.resume();
}

TEST_F(DbuctRpHeuristicRolloutFrameHubTest, PopForwardsMultipleBaseYields) {
    resolution_lineage a{nullptr, 0};
    resolution_lineage b{nullptr, 1};

    {
        ::testing::InSequence seq;
        EXPECT_CALL(pop_map, pop_frame());
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

TEST_F(DbuctRpHeuristicRolloutFrameHubTest, MapPoppedBeforeBaseYields) {
    resolution_lineage rl{nullptr, 0};
    bool map_popped = false;

    EXPECT_CALL(pop_map, pop_frame()).WillOnce([&]() { map_popped = true; });
    EXPECT_CALL(pop_base, pop_solver_frame())
        .WillOnce([&]() {
            EXPECT_TRUE(map_popped);
            return two_yield_base_pop(&rl, &rl);
        });

    drain(hub.pop_solver_frame());
}

TEST_F(DbuctRpHeuristicRolloutFrameHubTest, RepeatedPushPopCycles) {
    for (int i = 0; i < 3; ++i) {
        {
            ::testing::InSequence seq;
            EXPECT_CALL(push_base, push_solver_frame());
            EXPECT_CALL(push_map, push_frame());
        }
        hub.push_solver_frame();

        {
            ::testing::InSequence seq;
            EXPECT_CALL(pop_map, pop_frame());
            EXPECT_CALL(pop_base, pop_solver_frame())
                .WillOnce(::testing::Return(::testing::ByMove(empty_base_pop())));
        }
        drain(hub.pop_solver_frame());
    }
}
