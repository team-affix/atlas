// mcts_sim: wraps set_up_sim/tear_down_sim; owns MCTS tree; terminate score from i_compute_mcts_reward.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <vector>
#include "locator_fixture.hpp"
#include "infrastructure/mcts_sim.hpp"
#include "infrastructure/set_up_sim.hpp"
#include "infrastructure/tear_down_sim.hpp"
#include "interfaces/i_push_trail_frame.hpp"
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
#include "interfaces/i_clean_up_cdcl.hpp"
#include "interfaces/i_clear_chosen_goal_candidates.hpp"
#include "interfaces/i_compute_mcts_reward.hpp"
#include "value_objects/mcts_choice.hpp"

using ::testing::Return;

struct MockPushTrailFrame : public i_push_trail_frame {
    MOCK_METHOD(void, push, (), (override));
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

struct MockComputeMctsReward : public i_compute_mcts_reward {
    MOCK_METHOD(double, compute_mcts_reward, (), (const, override));
};

struct MockFrameAllocator : public i_frame_allocator {
    MOCK_METHOD(uint32_t, bump, (uint32_t), (override));
    MOCK_METHOD(uint32_t, peek, (), (const, override));
    MOCK_METHOD(void, reset, (), (override));
};

struct MockCleanUpCdcl : public i_clean_up_cdcl {
    MOCK_METHOD(void, cleanup, (), (override));
};

struct MockClearChosenGoalCandidates : public i_clear_chosen_goal_candidates {
    MOCK_METHOD(void, clear, (), (override));
};

struct MctsSimTest : public ::testing::Test {
    static constexpr double kExplorationConstant = 1.414;

    locator loc;
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
    set_up_sim inner_set_up;
    tear_down_sim inner_tear_down;
    mcts_sim sim;

    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};

    MctsSimTest() : inner_set_up(init_inner_set_up()), inner_tear_down(init_inner_tear_down()), sim(init_sim()) {
        ON_CALL(compute_mcts_reward, compute_mcts_reward()).WillByDefault(Return(0.0));
    }

    void bind_tear_down_deps() {
        loc.bind_as<i_pop_trail_frame>(pop_trail_frame);
        loc.bind_as<i_clear_unit_goals>(clear_unit_goals);
        loc.bind_as<i_clear_recorded_decisions>(clear_recorded_decisions);
        loc.bind_as<i_clear_recorded_resolutions>(clear_recorded_resolutions);
        loc.bind_as<i_clear_goal_candidate_rule_ids>(clear_goal_candidate_rule_ids);
        loc.bind_as<i_clear_goal_exprs>(clear_goal_exprs);
        loc.bind_as<i_clear_active_goals>(clear_active_goals);
        loc.bind_as<i_clear_candidate_frame_offsets>(clear_candidate_frame_offsets);
        loc.bind_as<i_clear_mhu_heads>(clear_mhu_heads);
        loc.bind_as<i_clear_bindings>(clear_bindings);
        loc.bind_as<i_trim_unpinned_lineages>(trim_unpinned_lineages);
        loc.bind_as<i_frame_allocator>(frame_allocator);
        loc.bind_as<i_clean_up_cdcl>(clean_up_cdcl);
        loc.bind_as<i_clear_chosen_goal_candidates>(clear_chosen_goal_candidates);
    }

    set_up_sim init_inner_set_up() {
        loc.bind_as<i_push_trail_frame>(push_trail_frame);
        return set_up_sim{loc};
    }

    tear_down_sim init_inner_tear_down() {
        bind_tear_down_deps();
        return tear_down_sim{loc};
    }

    mcts_sim init_sim() {
        loc.bind_as<i_compute_mcts_reward>(compute_mcts_reward);
        return mcts_sim{loc, inner_set_up, inner_tear_down, rng, kExplorationConstant};
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
