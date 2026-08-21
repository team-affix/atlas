// run_sim: the resolution loop. Owns the entire sim_termination decision --
// solved when the frontier empties, conflicted on a failed activation, a failed
// resolve or a detected conflict, depth_exceeded once max_resolutions is hit.
// Each iteration takes its resolution from the unit-goal queue when one is
// pending and from the decision generator otherwise.

#include <optional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
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

struct MockActivateInitialGoalsAndCandidates {
    MOCK_METHOD(bool, activate_initial_goals_and_candidates, ());
};

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

struct MockRecorder {
    MOCK_METHOD(void, record_decision_resolution, (const resolution_lineage*));
    MOCK_METHOD(void, record_unit_resolution, (const resolution_lineage*));
};

struct MockGetResolutionCount {
    MOCK_METHOD(size_t, get_resolution_count, (), (const));
};

} // namespace

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

struct RunSimTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 2;

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

    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, 0};

    test_run_sim_t make_run_sim(size_t max_resolutions) {
        return test_run_sim_t{
            activate_initial_goals_and_candidates,
            solution_detector, conflict_detector, unit_goal_detector,
            push_unit_goal, pop_unit_goal, decision_generator,
            elimination_generator, elimination_router, resolver,
            get_unit_resolution, recorder, recorder,
            get_resolution_count, max_resolutions};
    }

    test_run_sim_t sut{make_run_sim(kMaxResolutions)};

    void SetUp() override {
        ON_CALL(activate_initial_goals_and_candidates,
                activate_initial_goals_and_candidates()).WillByDefault(Return(true));
        // Mirror production: get_resolution_count() tracks how many resolutions
        // have been recorded so far, so the run loop is bounded by
        // max_resolutions. Every resolution -- decision or unit -- records once.
        ON_CALL(recorder, record_decision_resolution(testing::_))
            .WillByDefault([this](const resolution_lineage*) { ++resolution_count_; });
        ON_CALL(recorder, record_unit_resolution(testing::_))
            .WillByDefault([this](const resolution_lineage*) { ++resolution_count_; });
        ON_CALL(get_resolution_count, get_resolution_count())
            .WillByDefault([this] { return resolution_count_; });
    }
};

TEST_F(RunSimTest, ReturnsSolvedWhenFrontierEmpties) {
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates())
        .WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(true));
    EXPECT_CALL(decision_generator, generate()).Times(0);
    EXPECT_EQ(sut.run(), sim_termination::solved);
}

TEST_F(RunSimTest, ReturnsConflictedWhenInitialGoalsFail) {
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates())
        .WillOnce(Return(false));
    EXPECT_CALL(solution_detector, detect()).Times(0);
    EXPECT_CALL(decision_generator, generate()).Times(0);
    EXPECT_EQ(sut.run(), sim_termination::conflicted);
}

TEST_F(RunSimTest, ReturnsDepthExceededAtMaxResolutions) {
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates())
        .WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));

    EXPECT_EQ(sut.run(), sim_termination::depth_exceeded);
    EXPECT_EQ(resolution_count_, kMaxResolutions);
}

TEST_F(RunSimTest, ReturnsConflictedWhenResolverFails) {
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates())
        .WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillOnce(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl)).WillOnce([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillOnce(Return(false));
    EXPECT_EQ(sut.run(), sim_termination::conflicted);
}

TEST_F(RunSimTest, ReturnsConflictedWhenConflictDetected) {
    // A conflict must abandon the iteration outright: resolving the goal that
    // was just proved unsatisfiable would extend a dead branch.
    resolution_lineage elim_rl{&gl, 1};

    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates())
        .WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillOnce(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillOnce([&] { return single_elimination(&elim_rl); });
    EXPECT_CALL(elimination_router, route(&elim_rl))
        .WillOnce(Return(elimination_result::eliminated));
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(true));
    EXPECT_CALL(resolver, resolve).Times(0);

    EXPECT_EQ(sut.run(), sim_termination::conflicted);
}

TEST_F(RunSimTest, RoutesEliminationBeforeResolve) {
    resolution_lineage elim_rl{&gl, 1};

    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates())
        .WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillOnce([&] { return single_elimination(&elim_rl); })
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(elimination_router, route(&elim_rl))
        .WillOnce(Return(elimination_result::eliminated));
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_CALL(unit_goal_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));

    EXPECT_EQ(sut.run(), sim_termination::depth_exceeded);
}

TEST_F(RunSimTest, PushesUnitGoalWhenEliminationParentIsUnit) {
    resolution_lineage elim_rl{&gl, 1};

    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates())
        .WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillOnce([&] { return single_elimination(&elim_rl); })
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(elimination_router, route(&elim_rl))
        .WillOnce(Return(elimination_result::eliminated));
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(false));
    EXPECT_CALL(unit_goal_detector, detect(&gl)).WillOnce(Return(true));
    EXPECT_CALL(push_unit_goal, push(&gl)).Times(1);
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));

    EXPECT_EQ(sut.run(), sim_termination::depth_exceeded);
}

TEST_F(RunSimTest, RecordsDecisionWhenGeneratorChoosesResolution) {
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates())
        .WillOnce(Return(true));
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(recorder, record_decision_resolution(&rl)).Times(kMaxResolutions);
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));

    EXPECT_EQ(sut.run(), sim_termination::depth_exceeded);
}

TEST_F(RunSimTest, DrainsUnitGoalQueueBeforeConsultingDecisionGenerator) {
    // Unit goals have exactly one surviving candidate, so taking them first is
    // what keeps the search deterministic where no choice exists. Consulting
    // the decision generator while the queue is non-empty would turn a forced
    // resolution into a branching point and record it as a decision, inflating
    // the CDCL decision depth for everything below it.
    resolution_lineage unit_rl{&gl, 5};

    testing::InSequence seq;
    EXPECT_CALL(activate_initial_goals_and_candidates, activate_initial_goals_and_candidates())
        .WillOnce(Return(true));

    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillOnce(Return(&gl));
    EXPECT_CALL(get_unit_resolution, get(&gl)).WillOnce(Return(&unit_rl));
    EXPECT_CALL(recorder, record_unit_resolution(&unit_rl)).Times(1);
    EXPECT_CALL(elimination_generator, constrain(&unit_rl))
        .WillOnce([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&unit_rl)).WillOnce(Return(true));

    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillOnce(Return(&rl));
    EXPECT_CALL(recorder, record_decision_resolution(&rl)).Times(1);
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillOnce([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillOnce(Return(true));

    EXPECT_EQ(sut.run(), sim_termination::depth_exceeded);
}
