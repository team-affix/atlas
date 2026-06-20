// mcts_sim: wraps set_up_sim/tear_down_sim; owns MCTS tree; terminate score from compute_mcts_reward.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <vector>
#include "infrastructure/mcts_sim.hpp"
#include "infrastructure/set_up_sim.hpp"
#include "infrastructure/tear_down_sim.hpp"
#include "value_objects/mcts_choice.hpp"

using ::testing::Return;

struct MockPushTrailFrame {
    MOCK_METHOD(void, push, ());
};

struct MockPopTrailFrame {
    MOCK_METHOD(void, pop, ());
};

struct MockClearUnitGoals {
    MOCK_METHOD(void, clear, ());
};

struct MockClearRecordedDecisions {
    MOCK_METHOD(void, clear_recorded_decisions, ());
};

struct MockClearRecordedResolutions {
    MOCK_METHOD(void, clear_recorded_resolutions, ());
};

struct MockClearGoalCandidateRuleIds {
    MOCK_METHOD(void, clear_goal_candidate_rule_ids, ());
};

struct MockClearGoalExprs {
    MOCK_METHOD(void, clear_goal_exprs, ());
};

struct MockClearActiveGoals {
    MOCK_METHOD(void, clear_active_goals, ());
};

struct MockClearCandidateFrameOffsets {
    MOCK_METHOD(void, clear_candidate_frame_offsets, ());
};

struct MockClearMhuHeads {
    MOCK_METHOD(void, clear_mhu_heads, ());
};

struct MockClearBindings {
    MOCK_METHOD(void, clear_bindings, ());
};

struct MockTrimUnpinnedLineages {
    MOCK_METHOD(void, trim, ());
};

struct MockComputeMctsReward {
    MOCK_METHOD(double, compute_mcts_reward, (), (const));
};

struct MockFrameAllocator {
    MOCK_METHOD(uint32_t, bump, (uint32_t));
    MOCK_METHOD(uint32_t, peek, (), (const));
    MOCK_METHOD(void, reset, ());
};

struct MockCleanUpCdcl {
    MOCK_METHOD(void, cleanup, ());
};

struct MockClearChosenGoalCandidates {
    MOCK_METHOD(void, clear, ());
};

using TestSetUpSim = set_up_sim<MockPushTrailFrame>;
using TestTearDownSim = tear_down_sim<
    MockPopTrailFrame, MockClearUnitGoals, MockClearRecordedDecisions,
    MockClearRecordedResolutions, MockClearGoalCandidateRuleIds, MockClearGoalExprs,
    MockClearActiveGoals, MockClearCandidateFrameOffsets, MockClearMhuHeads,
    MockClearBindings, MockTrimUnpinnedLineages, MockFrameAllocator,
    MockCleanUpCdcl, MockClearChosenGoalCandidates>;
using TestMctsSim = mcts_sim<TestSetUpSim, TestTearDownSim, MockComputeMctsReward>;

struct MctsSimTest : public ::testing::Test {
    static constexpr double kExplorationConstant = 1.414;

    MockPushTrailFrame push_trail_frame;
    MockPopTrailFrame pop_trail_frame;
    testing::NiceMock<MockClearUnitGoals> clear_unit_goals;
    testing::NiceMock<MockClearRecordedDecisions> clear_recorded_decisions;
    testing::NiceMock<MockClearRecordedResolutions> clear_recorded_resolutions;
    testing::NiceMock<MockClearGoalCandidateRuleIds> clear_goal_candidate_rule_ids;
    testing::NiceMock<MockClearGoalExprs> clear_goal_exprs;
    testing::NiceMock<MockClearActiveGoals> clear_active_goals;
    testing::NiceMock<MockClearCandidateFrameOffsets> clear_candidate_frame_offsets;
    testing::NiceMock<MockClearMhuHeads> clear_mhu_heads;
    testing::NiceMock<MockClearBindings> clear_bindings;
    testing::NiceMock<MockTrimUnpinnedLineages> trim_unpinned_lineages;
    testing::NiceMock<MockFrameAllocator> frame_allocator;
    testing::NiceMock<MockCleanUpCdcl> clean_up_cdcl;
    testing::NiceMock<MockClearChosenGoalCandidates> clear_chosen_goal_candidates;
    MockComputeMctsReward compute_mcts_reward;
    std::mt19937 rng{42};

    TestSetUpSim inner_set_up{push_trail_frame};
    TestTearDownSim inner_tear_down{
        pop_trail_frame, clear_unit_goals, clear_recorded_decisions,
        clear_recorded_resolutions, clear_goal_candidate_rule_ids, clear_goal_exprs,
        clear_active_goals, clear_candidate_frame_offsets, clear_mhu_heads,
        clear_bindings, trim_unpinned_lineages, frame_allocator,
        clean_up_cdcl, clear_chosen_goal_candidates};
    TestMctsSim sim{inner_set_up, inner_tear_down, compute_mcts_reward,
                    rng, kExplorationConstant};

    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};

    void SetUp() override {
        ON_CALL(compute_mcts_reward, compute_mcts_reward()).WillByDefault(Return(0.0));
    }
};

TEST_F(MctsSimTest, SetUpDelegatesToInnerSetUpSim) {
    testing::InSequence seq;
    EXPECT_CALL(push_trail_frame, push()).Times(1);
    EXPECT_CALL(compute_mcts_reward, compute_mcts_reward()).Times(1);
    EXPECT_CALL(pop_trail_frame, pop()).Times(1);
    sim.set_up();
    sim.tear_down();
}

TEST_F(MctsSimTest, TearDownQueriesMctsRewardBeforeInnerTearDown) {
    testing::InSequence seq;
    EXPECT_CALL(push_trail_frame, push()).Times(1);
    sim.set_up();
    EXPECT_CALL(compute_mcts_reward, compute_mcts_reward()).WillOnce(Return(-5.0));
    EXPECT_CALL(pop_trail_frame, pop()).Times(1);
    sim.tear_down();
}

TEST_F(MctsSimTest, TearDownDelegatesFullClearSequenceToInnerTearDown) {
    EXPECT_CALL(push_trail_frame, push()).Times(1);
    sim.set_up();
    EXPECT_CALL(compute_mcts_reward, compute_mcts_reward()).WillOnce(Return(0.0));
    EXPECT_CALL(pop_trail_frame, pop()).Times(1);
    EXPECT_CALL(clear_unit_goals, clear()).Times(1);
    EXPECT_CALL(clear_recorded_decisions, clear_recorded_decisions()).Times(1);
    EXPECT_CALL(clear_recorded_resolutions, clear_recorded_resolutions()).Times(1);
    EXPECT_CALL(clear_goal_candidate_rule_ids, clear_goal_candidate_rule_ids()).Times(1);
    EXPECT_CALL(clear_goal_exprs, clear_goal_exprs()).Times(1);
    EXPECT_CALL(clear_active_goals, clear_active_goals()).Times(1);
    EXPECT_CALL(clear_candidate_frame_offsets, clear_candidate_frame_offsets()).Times(1);
    EXPECT_CALL(clear_mhu_heads, clear_mhu_heads()).Times(1);
    EXPECT_CALL(clear_bindings, clear_bindings()).Times(1);
    EXPECT_CALL(frame_allocator, reset()).Times(1);
    EXPECT_CALL(clean_up_cdcl, cleanup()).Times(1);
    EXPECT_CALL(clear_chosen_goal_candidates, clear()).Times(1);
    EXPECT_CALL(trim_unpinned_lineages, trim()).Times(1);
    sim.tear_down();
}

TEST_F(MctsSimTest, ChooseAfterSetUpReturnsOneOfSuppliedGoalChoices) {
    EXPECT_CALL(push_trail_frame, push()).Times(1);
    sim.set_up();
    std::vector<mcts_choice> choices{mcts_choice{&gl0}, mcts_choice{&gl1}};
    mcts_choice picked = sim.choose(choices);
    const auto* picked_gl = std::get_if<const goal_lineage*>(&picked);
    ASSERT_NE(picked_gl, nullptr);
    EXPECT_TRUE(*picked_gl == &gl0 || *picked_gl == &gl1);
    EXPECT_CALL(compute_mcts_reward, compute_mcts_reward()).Times(1);
    EXPECT_CALL(pop_trail_frame, pop()).Times(1);
    sim.tear_down();
}

TEST_F(MctsSimTest, ChooseAfterSetUpReturnsOneOfSuppliedRuleChoices) {
    EXPECT_CALL(push_trail_frame, push()).Times(1);
    sim.set_up();
    std::vector<mcts_choice> choices{mcts_choice{rule_id{0}}, mcts_choice{rule_id{1}}};
    mcts_choice picked = sim.choose(choices);
    const auto* picked_rule = std::get_if<rule_id>(&picked);
    ASSERT_NE(picked_rule, nullptr);
    EXPECT_TRUE(*picked_rule == 0 || *picked_rule == 1);
    EXPECT_CALL(compute_mcts_reward, compute_mcts_reward()).Times(1);
    EXPECT_CALL(pop_trail_frame, pop()).Times(1);
    sim.tear_down();
}

TEST_F(MctsSimTest, MultipleRolloutChoosesStayWithinChoiceSet) {
    EXPECT_CALL(push_trail_frame, push()).Times(1);
    sim.set_up();
    std::vector<mcts_choice> choices{mcts_choice{&gl0}, mcts_choice{&gl1}};
    for (int i = 0; i < 10; ++i) {
        mcts_choice picked = sim.choose(choices);
        const auto* picked_gl = std::get_if<const goal_lineage*>(&picked);
        ASSERT_NE(picked_gl, nullptr);
        EXPECT_TRUE(*picked_gl == &gl0 || *picked_gl == &gl1);
    }
    EXPECT_CALL(compute_mcts_reward, compute_mcts_reward()).Times(1);
    EXPECT_CALL(pop_trail_frame, pop()).Times(1);
    sim.tear_down();
}

TEST_F(MctsSimTest, FullLifecycleSetUpChooseTearDownTwice) {
    std::vector<mcts_choice> choices{mcts_choice{rule_id{0}}, mcts_choice{rule_id{1}}};
    for (int cycle = 0; cycle < 2; ++cycle) {
        testing::InSequence seq;
        EXPECT_CALL(push_trail_frame, push()).Times(1);
        sim.set_up();
        mcts_choice picked = sim.choose(choices);
        EXPECT_TRUE(std::holds_alternative<rule_id>(picked));
        EXPECT_CALL(compute_mcts_reward, compute_mcts_reward()).WillOnce(Return(-static_cast<double>(cycle)));
        EXPECT_CALL(pop_trail_frame, pop()).Times(1);
        sim.tear_down();
    }
}
