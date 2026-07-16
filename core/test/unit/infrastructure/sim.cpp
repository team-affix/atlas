// Simulation loop: solution/conflict/depth termination, elimination routing, and
// resolution stepping. set_up pushes a trail frame; run() activates initial goals
// then enters the resolution loop. tear_down pops the trail and clears stores.
// run() must honor max_resolutions and short-circuit on solution_detector.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/set_up_sim.hpp"
#include "infrastructure/tear_down_sim.hpp"
#include "infrastructure/run_sim.hpp"
#include "infrastructure/coroutine.hpp"
#include "value_objects/elimination_result.hpp"

using ::testing::Return;

namespace {

coroutine<const resolution_lineage*, void> empty_eliminations() {
    co_return;
}

coroutine<const resolution_lineage*, void> single_elimination(const resolution_lineage* elim) {
    co_yield elim;
    co_return;
}

} // namespace

struct MockSolutionDetector {
    MOCK_METHOD(bool, detect, (), (const));
};

struct MockConflictDetector {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (const));
};

struct MockUnitGoalDetector {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (const));
};

struct MockPushUnitGoal {
    MOCK_METHOD(void, push, (const goal_lineage*));
};

struct MockPopUnitGoal {
    MOCK_METHOD(std::optional<const goal_lineage*>, pop, ());
};

struct MockGenerateDecision {
    MOCK_METHOD(const resolution_lineage*, generate, ());
};

struct MockEliminationGenerator {
    MOCK_METHOD((coroutine<const resolution_lineage*, void>), constrain, (const resolution_lineage*));
};

struct MockEliminationRouter {
    MOCK_METHOD(elimination_result, route, (const resolution_lineage*));
};

struct MockResolver {
    MOCK_METHOD(bool, resolve, (const resolution_lineage*));
};

struct MockGetUnitResolution {
    MOCK_METHOD(const resolution_lineage*, get, (const goal_lineage*));
};

struct MockPushFrame {
    MOCK_METHOD(void, push_frame, ());
};

struct MockPopFrame {
    MOCK_METHOD(void, pop_frame, ());
};

struct MockActivateInitialGoalsAndCandidates {
    MOCK_METHOD(bool, activate_initial_goals_and_candidates, ());
};

struct MockRecorder {
    MOCK_METHOD(void, record_decision_resolution, (const resolution_lineage*));
    MOCK_METHOD(void, record_unit_resolution, (const resolution_lineage*));
};

struct MockGetResolutionCount {
    MOCK_METHOD(size_t, get_resolution_count, (), (const));
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

using test_set_up_sim_t = set_up_sim<MockPushFrame>;
using test_run_sim_t = run_sim<
    MockActivateInitialGoalsAndCandidates,
    MockSolutionDetector,
    MockConflictDetector,
    MockUnitGoalDetector,
    MockPushUnitGoal,
    MockPopUnitGoal,
    MockGenerateDecision,
    MockEliminationGenerator,
    MockEliminationRouter,
    MockResolver,
    MockGetUnitResolution,
    testing::NiceMock<MockRecorder>,
    testing::NiceMock<MockRecorder>,
    testing::NiceMock<MockGetResolutionCount>>;
using test_tear_down_sim_t = tear_down_sim<
    MockPopFrame,
    testing::NiceMock<MockClearUnitGoals>,
    testing::NiceMock<MockClearRecordedDecisions>,
    testing::NiceMock<MockClearRecordedResolutions>,
    testing::NiceMock<MockClearGoalCandidateRuleIds>,
    testing::NiceMock<MockClearGoalExprs>,
    testing::NiceMock<MockClearActiveGoals>,
    testing::NiceMock<MockClearCandidateFrameOffsets>,
    testing::NiceMock<MockClearMhuHeads>,
    testing::NiceMock<MockClearBindings>,
    testing::NiceMock<MockTrimUnpinnedLineages>,
    testing::NiceMock<MockFrameAllocator>,
    testing::NiceMock<MockCleanUpCdcl>,
    testing::NiceMock<MockClearChosenGoalCandidates>>;

struct SimTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 2;

    MockPushFrame push_frame;
    MockPopFrame pop_frame;
    MockActivateInitialGoalsAndCandidates activate_initial_goals_and_candidates;
    MockSolutionDetector solution_detector;
    MockConflictDetector conflict_detector;
    MockUnitGoalDetector unit_goal_detector;
    MockPushUnitGoal push_unit_goal;
    MockPopUnitGoal pop_unit_goal;
    MockGenerateDecision decision_generator;
    MockEliminationGenerator elimination_generator;
    MockEliminationRouter elimination_router;
    MockResolver resolver;
    MockGetUnitResolution get_unit_resolution;
    testing::NiceMock<MockRecorder> recorder;
    testing::NiceMock<MockGetResolutionCount> get_resolution_count;
    size_t resolution_count_ = 0;
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

    test_set_up_sim_t set_up_sim_{push_frame};
    test_tear_down_sim_t tear_down_sim_{
        pop_frame, clear_unit_goals, clear_recorded_decisions,
        clear_recorded_resolutions, clear_goal_candidate_rule_ids, clear_goal_exprs,
        clear_active_goals, clear_candidate_frame_offsets, clear_mhu_heads,
        clear_bindings, trim_unpinned_lineages, frame_allocator,
        clean_up_cdcl, clear_chosen_goal_candidates};

    test_run_sim_t make_run_sim(size_t max_resolutions) {
        return test_run_sim_t{
            activate_initial_goals_and_candidates,
            solution_detector, conflict_detector, unit_goal_detector,
            push_unit_goal, pop_unit_goal, decision_generator,
            elimination_generator, elimination_router, resolver,
            get_unit_resolution, recorder, recorder,
            get_resolution_count, max_resolutions};
    }

    test_run_sim_t run_sim_{make_run_sim(kMaxResolutions)};

    void set_up() { set_up_sim_.set_up(); }
    sim_termination run() { return run_sim_.run(); }
    void tear_down() { tear_down_sim_.tear_down(); }

    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, 0};

    void SetUp() override {
        ON_CALL(activate_initial_goals_and_candidates,
                activate_initial_goals_and_candidates()).WillByDefault(Return(true));
        // Mirror production: get_resolution_count() tracks how many resolutions
        // have been recorded so far, so the run loop is bounded by max_resolutions.
        // Every resolution -- decision or unit -- records exactly once.
        ON_CALL(recorder, record_decision_resolution(testing::_))
            .WillByDefault([this](const resolution_lineage*) { ++resolution_count_; });
        ON_CALL(recorder, record_unit_resolution(testing::_))
            .WillByDefault([this](const resolution_lineage*) { ++resolution_count_; });
        ON_CALL(get_resolution_count, get_resolution_count())
            .WillByDefault([this] { return resolution_count_; });
    }
};

TEST_F(SimTest, RunReturnsSolvedWhenDetectorSaysSo) {
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(true));
    EXPECT_CALL(decision_generator, generate()).Times(0);
    EXPECT_EQ(run(), sim_termination::solved);
}

TEST_F(SimTest, RunReturnsDepthExceededWhenNoSolutionWithinLimit) {
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));
    EXPECT_EQ(run(), sim_termination::depth_exceeded);
}

TEST_F(SimTest, RunReturnsConflictedWhenResolverFails) {
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillOnce(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl)).WillOnce([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillOnce(Return(false));
    EXPECT_EQ(run(), sim_termination::conflicted);
}

TEST_F(SimTest, RunReturnsConflictedWhenInitialGoalsFail) {
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates()).WillOnce(Return(false));
    EXPECT_CALL(solution_detector, detect()).Times(0);
    EXPECT_CALL(decision_generator, generate()).Times(0);
    EXPECT_EQ(run(), sim_termination::conflicted);
}

TEST_F(SimTest, TearDownPopsFrameAndClearsNonBacktrackedStores) {
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

    tear_down();
}

TEST_F(SimTest, SetUpPushesFrameOnly) {
    EXPECT_CALL(push_frame, push_frame()).Times(1);
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates()).Times(0);
    set_up();
    tear_down();
}

TEST_F(SimTest, RunRoutesEliminationBeforeResolve) {
    resolution_lineage elim_rl{&gl, 1};

    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillOnce([&] { return single_elimination(&elim_rl); })
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(elimination_router, route(&elim_rl)).Times(1);
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_CALL(unit_goal_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));

    EXPECT_EQ(run(), sim_termination::depth_exceeded);
}

TEST_F(SimTest, RunReturnsConflictedWhenEliminationParentConflicts) {
    resolution_lineage elim_rl{&gl, 1};

    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillOnce(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillOnce([&] { return single_elimination(&elim_rl); });
    EXPECT_CALL(elimination_router, route(&elim_rl)).Times(1);
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(true));
    EXPECT_CALL(resolver, resolve).Times(0);

    EXPECT_EQ(run(), sim_termination::conflicted);
}

TEST_F(SimTest, RunPushesUnitGoalWhenEliminationParentIsUnit) {
    resolution_lineage elim_rl{&gl, 1};

    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillOnce([&] { return single_elimination(&elim_rl); })
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(elimination_router, route(&elim_rl)).Times(1);
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_CALL(unit_goal_detector, detect(&gl)).WillOnce(Return(true));
    EXPECT_CALL(push_unit_goal, push(&gl)).Times(1);
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));

    EXPECT_EQ(run(), sim_termination::depth_exceeded);
}

TEST_F(SimTest, RecordsDecisionWhenGeneratorChoosesResolution) {
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(recorder, record_decision_resolution(&rl)).Times(2);
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));

    EXPECT_EQ(run(), sim_termination::depth_exceeded);
}

TEST_F(SimTest, RunUsesPoppedUnitGoalForNextResolution) {
    resolution_lineage unit_rl{&gl, 5};

    test_run_sim_t one_resolution = make_run_sim(1);

    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates()).WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillOnce(Return(&gl));
    EXPECT_CALL(get_unit_resolution, get(&gl)).WillOnce(Return(&unit_rl));
    EXPECT_CALL(decision_generator, generate()).Times(0);
    EXPECT_CALL(recorder, record_decision_resolution).Times(0);
    EXPECT_CALL(recorder, record_unit_resolution(&unit_rl)).Times(1);
    EXPECT_CALL(elimination_generator, constrain(&unit_rl))
        .WillOnce([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&unit_rl)).WillOnce(Return(true));

    EXPECT_EQ(one_resolution.run(), sim_termination::depth_exceeded);
}
