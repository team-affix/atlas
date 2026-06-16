// horizon_tear_down_sim: clears goal weights and CGW before base tear_down.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "locator_fixture.hpp"
#include "infrastructure/horizon_tear_down_sim.hpp"
#include "interfaces/i_clear_goal_weights.hpp"
#include "interfaces/i_clear_grounded_weight.hpp"
#include "interfaces/i_pop_trail_frame.hpp"
#include "interfaces/i_clear_unit_goals.hpp"
#include "interfaces/i_clear_recorded_decisions.hpp"
#include "interfaces/i_clear_recorded_resolutions.hpp"
#include "interfaces/i_clear_goal_candidate_rule_ids.hpp"
#include "interfaces/i_clear_goal_exprs.hpp"
#include "interfaces/i_clear_active_goals.hpp"
#include "interfaces/i_clear_candidate_frame_offsets.hpp"
#include "interfaces/i_clear_mhu_heads.hpp"
#include "interfaces/i_clear_bindings.hpp"
#include "interfaces/i_trim_unpinned_lineages.hpp"
#include "interfaces/i_frame_allocator.hpp"

using ::testing::NiceMock;

struct MockClearGoalWeights : public i_clear_goal_weights {
    MOCK_METHOD(void, clear_goal_weights, (), (override));
};

struct MockClearGroundedWeight : public i_clear_grounded_weight {
    MOCK_METHOD(void, clear, (), (override));
};

struct MockPopTrailFrame : public i_pop_trail_frame {
    MOCK_METHOD(void, pop, (), (override));
};

struct MockClearUnitGoals : public i_clear_unit_goals {
    MOCK_METHOD(void, clear, (), (override));
};

struct MockClearRecordedDecisions : public i_clear_recorded_decisions {
    MOCK_METHOD(void, clear_recorded_decisions, (), (override));
};

struct MockClearRecordedResolutions : public i_clear_recorded_resolutions {
    MOCK_METHOD(void, clear_recorded_resolutions, (), (override));
};

struct MockClearGoalCandidateRuleIds : public i_clear_goal_candidate_rule_ids {
    MOCK_METHOD(void, clear_goal_candidate_rule_ids, (), (override));
};

struct MockClearGoalExprs : public i_clear_goal_exprs {
    MOCK_METHOD(void, clear_goal_exprs, (), (override));
};

struct MockClearActiveGoals : public i_clear_active_goals {
    MOCK_METHOD(void, clear_active_goals, (), (override));
};

struct MockClearCandidateFrameOffsets : public i_clear_candidate_frame_offsets {
    MOCK_METHOD(void, clear_candidate_frame_offsets, (), (override));
};

struct MockClearMhuHeads : public i_clear_mhu_heads {
    MOCK_METHOD(void, clear_mhu_heads, (), (override));
};

struct MockClearBindings : public i_clear_bindings {
    MOCK_METHOD(void, clear_bindings, (), (override));
};

struct MockTrimUnpinnedLineages : public i_trim_unpinned_lineages {
    MOCK_METHOD(void, trim, (), (override));
};

struct MockFrameAllocator : public i_frame_allocator {
    MOCK_METHOD(uint32_t, bump, (uint32_t), (override));
    MOCK_METHOD(uint32_t, peek, (), (const, override));
    MOCK_METHOD(void, reset, (), (override));
};

struct HorizonTearDownSimTest : public ::testing::Test {
    locator loc;
    MockClearGoalWeights clear_goal_weights;
    MockClearGroundedWeight clear_grounded_weight;
    NiceMock<MockPopTrailFrame> pop_trail;
    NiceMock<MockClearUnitGoals> clear_unit_goals;
    NiceMock<MockClearRecordedDecisions> clear_decisions;
    NiceMock<MockClearRecordedResolutions> clear_resolutions;
    NiceMock<MockClearGoalCandidateRuleIds> clear_candidates;
    NiceMock<MockClearGoalExprs> clear_exprs;
    NiceMock<MockClearActiveGoals> clear_active;
    NiceMock<MockClearCandidateFrameOffsets> clear_frame_offsets;
    NiceMock<MockClearMhuHeads> clear_mhu;
    NiceMock<MockClearBindings> clear_bindings;
    NiceMock<MockTrimUnpinnedLineages> trim_lineages;
    NiceMock<MockFrameAllocator> frame_allocator;
    std::unique_ptr<horizon_tear_down_sim> tear_down;

    void SetUp() override {
        loc.bind_as<i_clear_goal_weights>(clear_goal_weights);
        loc.bind_as<i_clear_grounded_weight>(clear_grounded_weight);
        loc.bind_as<i_pop_trail_frame>(pop_trail);
        loc.bind_as<i_clear_unit_goals>(clear_unit_goals);
        loc.bind_as<i_clear_recorded_decisions>(clear_decisions);
        loc.bind_as<i_clear_recorded_resolutions>(clear_resolutions);
        loc.bind_as<i_clear_goal_candidate_rule_ids>(clear_candidates);
        loc.bind_as<i_clear_goal_exprs>(clear_exprs);
        loc.bind_as<i_clear_active_goals>(clear_active);
        loc.bind_as<i_clear_candidate_frame_offsets>(clear_frame_offsets);
        loc.bind_as<i_clear_mhu_heads>(clear_mhu);
        loc.bind_as<i_clear_bindings>(clear_bindings);
        loc.bind_as<i_trim_unpinned_lineages>(trim_lineages);
        loc.bind_as<i_frame_allocator>(frame_allocator);
        tear_down = std::make_unique<horizon_tear_down_sim>(loc);
    }
};

TEST_F(HorizonTearDownSimTest, ClearsWeightsAndCgwBeforeBaseTearDown) {
    testing::InSequence seq;
    EXPECT_CALL(clear_goal_weights, clear_goal_weights()).Times(1);
    EXPECT_CALL(clear_grounded_weight, clear()).Times(1);
    EXPECT_CALL(pop_trail, pop()).Times(1);
    tear_down->tear_down();
}
