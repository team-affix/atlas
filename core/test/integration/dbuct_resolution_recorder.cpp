// Integration: dbuct_resolution_recorder fanning run_sim's per-step recording out
// to REAL dbuct_decision_memory, dbuct_resolution_memory, dbuct_nearest_decision
// and dbuct_avoidance_unit_boundary.
//
// The recorder's unit test mocks all five slots, so it proves the calls happen
// but not that the oracles end up mutually consistent. That consistency is the
// actual contract: the boundary asks nearest_decision about the resolution's
// grandparent WHILE the recorder is mid-fan-out, so the recorder's ordering
// (note_decision_resolution before log_decision) is load-bearing and only
// observable against the real oracles.
//
// Only the MCTS frame depth is mocked -- it belongs to the MCTS sim.

#include <cstddef>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"
#include "infrastructure/dbuct_decision_memory.hpp"
#include "infrastructure/dbuct_nearest_decision.hpp"
#include "infrastructure/dbuct_resolution_memory.hpp"
#include "infrastructure/dbuct_resolution_recorder.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;
using ::testing::ReturnPointee;

namespace {

struct MockGetMctsFrameDepth {
    MOCK_METHOD(size_t, depth, (), (const));
};

using boundary_t =
    dbuct_avoidance_unit_boundary<dbuct_nearest_decision, NiceMock<MockGetMctsFrameDepth>>;

using recorder_t = dbuct_resolution_recorder<
    dbuct_decision_memory, dbuct_resolution_memory,
    dbuct_nearest_decision, dbuct_nearest_decision, boundary_t>;

}  // namespace

struct DbuctResolutionRecorderIntegrationTest : public ::testing::Test {
    size_t mcts_frame_depth = 1;
    NiceMock<MockGetMctsFrameDepth> get_mcts_frame_depth;

    dbuct_decision_memory decision_memory;
    dbuct_resolution_memory resolution_memory;
    dbuct_nearest_decision nearest_decision;
    boundary_t avoidance_unit_boundary{nearest_decision, get_mcts_frame_depth};

    recorder_t recorder{decision_memory, resolution_memory,
                        nearest_decision, nearest_decision, avoidance_unit_boundary};

    // Two root decisions on disjoint branches, plus a unit resolution whose
    // grandparent is the first decision.
    goal_lineage g1{nullptr, 0};
    resolution_lineage d1{&g1, 0};
    goal_lineage g2{nullptr, 1};
    resolution_lineage d2{&g2, 0};
    goal_lineage gu{&d1, 0};
    resolution_lineage u{&gu, 0};

    void SetUp() override {
        ON_CALL(get_mcts_frame_depth, depth())
            .WillByDefault(ReturnPointee(&mcts_frame_depth));
    }

    // Mirrors dbuct_frame_hub's push order for just these four oracles.
    void push_frame() {
        decision_memory.push_frame();
        resolution_memory.push_frame();
        nearest_decision.push_frame();
        avoidance_unit_boundary.push_frame();
    }

    void pop_frame() {
        avoidance_unit_boundary.pop_frame();
        nearest_decision.pop_frame();
        resolution_memory.pop_frame();
        decision_memory.pop_frame();
    }
};

TEST_F(DbuctResolutionRecorderIntegrationTest, DecisionResolutionUpdatesAllFiveOracles) {
    mcts_frame_depth = 4;

    recorder.record_decision_resolution(&d1);

    EXPECT_EQ(decision_memory.count(), 1u);
    EXPECT_EQ(resolution_memory.get_resolution_count(), 1u);
    EXPECT_EQ(nearest_decision.get_nearest_decision(&d1), &d1);
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_decision(), &d1);
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_mcts_frame_depth(), 4u);
}

TEST_F(DbuctResolutionRecorderIntegrationTest, UnitResolutionSkipsDecisionMemoryAndBoundary) {
    // A unit misrecorded as a decision would rotate the boundary and let CDCL
    // learn at the wrong MCTS depth, pruning branches that still hold solutions.
    mcts_frame_depth = 4;
    recorder.record_decision_resolution(&d1);

    mcts_frame_depth = 9;
    recorder.record_unit_resolution(&u);

    EXPECT_EQ(resolution_memory.get_resolution_count(), 2u);
    EXPECT_EQ(decision_memory.count(), 1u);
    // The unit inherits its grandparent's nearest decision rather than becoming one.
    EXPECT_EQ(nearest_decision.get_nearest_decision(&u), &d1);
    // The boundary never saw the unit, so it still describes d1 at depth 4.
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_decision(), &d1);
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_mcts_frame_depth(), 4u);
    EXPECT_EQ(avoidance_unit_boundary.get_penultimate_decision(), nullptr);
}

TEST_F(DbuctResolutionRecorderIntegrationTest, PopFrameRestoresOracleStateAfterRecordedDecision) {
    mcts_frame_depth = 4;
    recorder.record_decision_resolution(&d1);

    push_frame();
    mcts_frame_depth = 7;
    recorder.record_decision_resolution(&d2);

    ASSERT_EQ(decision_memory.count(), 2u);
    ASSERT_EQ(avoidance_unit_boundary.get_ultimate_decision(), &d2);
    ASSERT_EQ(avoidance_unit_boundary.get_penultimate_decision(), &d1);
    ASSERT_EQ(avoidance_unit_boundary.get_penultimate_mcts_frame_depth(), 4u);

    pop_frame();

    EXPECT_EQ(decision_memory.count(), 1u);
    EXPECT_EQ(resolution_memory.get_resolution_count(), 1u);
    EXPECT_THROW(nearest_decision.get_nearest_decision(&d2), std::out_of_range);
    EXPECT_EQ(nearest_decision.get_nearest_decision(&d1), &d1);
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_decision(), &d1);
    EXPECT_EQ(avoidance_unit_boundary.get_ultimate_mcts_frame_depth(), 4u);
    EXPECT_EQ(avoidance_unit_boundary.get_penultimate_decision(), nullptr);
}
