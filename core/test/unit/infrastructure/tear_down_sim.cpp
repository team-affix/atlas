// tear_down_sim: mirror image of set_up_sim. Pops the solver frame first, then
// clears every store that is not restored by that pop.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/tear_down_sim.hpp"

using ::testing::Expectation;
using ::testing::NiceMock;

namespace {

struct MockPopFrame {
    MOCK_METHOD(void, pop_frame, ());
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

struct MockResetFrameAllocator {
    MOCK_METHOD(void, reset, ());
};

struct MockCleanUpCdcl {
    MOCK_METHOD(void, cleanup, ());
};

struct MockClearChosenGoalCandidates {
    MOCK_METHOD(void, clear, ());
};

} // namespace

using test_tear_down_sim_t = tear_down_sim<
    NiceMock<MockPopFrame>,
    NiceMock<MockClearUnitGoals>,
    NiceMock<MockClearRecordedDecisions>,
    NiceMock<MockClearRecordedResolutions>,
    NiceMock<MockClearGoalCandidateRuleIds>,
    NiceMock<MockClearGoalExprs>,
    NiceMock<MockClearActiveGoals>,
    NiceMock<MockClearCandidateFrameOffsets>,
    NiceMock<MockClearMhuHeads>,
    NiceMock<MockClearBindings>,
    NiceMock<MockTrimUnpinnedLineages>,
    NiceMock<MockResetFrameAllocator>,
    NiceMock<MockCleanUpCdcl>,
    NiceMock<MockClearChosenGoalCandidates>>;

struct TearDownSimTest : public ::testing::Test {
    NiceMock<MockPopFrame> pop_frame;
    NiceMock<MockClearUnitGoals> clear_unit_goals;
    NiceMock<MockClearRecordedDecisions> clear_recorded_decisions;
    NiceMock<MockClearRecordedResolutions> clear_recorded_resolutions;
    NiceMock<MockClearGoalCandidateRuleIds> clear_goal_candidate_rule_ids;
    NiceMock<MockClearGoalExprs> clear_goal_exprs;
    NiceMock<MockClearActiveGoals> clear_active_goals;
    NiceMock<MockClearCandidateFrameOffsets> clear_candidate_frame_offsets;
    NiceMock<MockClearMhuHeads> clear_mhu_heads;
    NiceMock<MockClearBindings> clear_bindings;
    NiceMock<MockTrimUnpinnedLineages> trim_unpinned_lineages;
    NiceMock<MockResetFrameAllocator> frame_allocator;
    NiceMock<MockCleanUpCdcl> clean_up_cdcl;
    NiceMock<MockClearChosenGoalCandidates> clear_chosen_goal_candidates;

    test_tear_down_sim_t sut{
        pop_frame, clear_unit_goals, clear_recorded_decisions,
        clear_recorded_resolutions, clear_goal_candidate_rule_ids, clear_goal_exprs,
        clear_active_goals, clear_candidate_frame_offsets, clear_mhu_heads,
        clear_bindings, trim_unpinned_lineages, frame_allocator,
        clean_up_cdcl, clear_chosen_goal_candidates};
};

TEST_F(TearDownSimTest, ClearsEveryStoreExactlyOnce) {
    // Each of these is state that survives the frame pop, so anything skipped
    // here leaks into the next simulation as a phantom goal, binding or watcher.
    EXPECT_CALL(pop_frame, pop_frame()).Times(1);
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

    sut.tear_down();
}

TEST_F(TearDownSimTest, PopsFrameBeforeClearingStores) {
    // The pop replays journaled undo actions against the stores it restores, so
    // clearing first would leave those undos writing into emptied maps. Only that
    // one relationship is asserted: the order the clears run in among themselves
    // is the source's business, not a contract.
    Expectation pop = EXPECT_CALL(pop_frame, pop_frame()).Times(1);
    EXPECT_CALL(clear_unit_goals, clear()).Times(1).After(pop);
    EXPECT_CALL(clear_recorded_decisions, clear_recorded_decisions()).Times(1).After(pop);
    EXPECT_CALL(clear_recorded_resolutions, clear_recorded_resolutions()).Times(1).After(pop);
    EXPECT_CALL(clear_goal_candidate_rule_ids, clear_goal_candidate_rule_ids()).Times(1).After(pop);
    EXPECT_CALL(clear_goal_exprs, clear_goal_exprs()).Times(1).After(pop);
    EXPECT_CALL(clear_active_goals, clear_active_goals()).Times(1).After(pop);
    EXPECT_CALL(clear_candidate_frame_offsets, clear_candidate_frame_offsets()).Times(1).After(pop);
    EXPECT_CALL(clear_mhu_heads, clear_mhu_heads()).Times(1).After(pop);
    EXPECT_CALL(clear_bindings, clear_bindings()).Times(1).After(pop);
    EXPECT_CALL(frame_allocator, reset()).Times(1).After(pop);
    EXPECT_CALL(clean_up_cdcl, cleanup()).Times(1).After(pop);
    EXPECT_CALL(clear_chosen_goal_candidates, clear()).Times(1).After(pop);
    EXPECT_CALL(trim_unpinned_lineages, trim()).Times(1).After(pop);

    sut.tear_down();
}
