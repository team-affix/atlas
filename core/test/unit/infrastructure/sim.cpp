// Simulation loop: solution/conflict/depth termination, elimination routing, and
// resolution stepping. set_up/tear_down are no-ops; run() must honor max_resolutions
// and short-circuit on solution_detector.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/sim.hpp"
#include "../../../core/hpp/infrastructure/rule_set.hpp"
#include "../../../core/hpp/interfaces/i_lineage_pool.hpp"
#include "../../../core/hpp/interfaces/i_solution_detector.hpp"
#include "../../../core/hpp/interfaces/i_conflict_detector.hpp"
#include "../../../core/hpp/interfaces/i_unit_goal_detector.hpp"
#include "../../../core/hpp/interfaces/i_push_unit_goal.hpp"
#include "../../../core/hpp/interfaces/i_pop_unit_goal.hpp"
#include "../../../core/hpp/interfaces/i_check_unit_goals_empty.hpp"
#include "../../../core/hpp/interfaces/i_decision_generator.hpp"
#include "../../../core/hpp/interfaces/i_elimination_generator.hpp"
#include "../../../core/hpp/interfaces/i_elimination_router.hpp"
#include "../../../core/hpp/interfaces/i_resolver.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_candidate_rules.hpp"
#include "../../../core/hpp/value_objects/elimination_result.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

namespace {

state_machine<const resolution_lineage*> empty_eliminations() {
    co_return;
}

}  // namespace

struct MockLineagePool : public i_lineage_pool {
    MOCK_METHOD((const goal_lineage*), goal, (const resolution_lineage*, subgoal_id), (override));
    MOCK_METHOD((const resolution_lineage*), resolution, (const goal_lineage*, rule_id), (override));
    MOCK_METHOD(void, pin_goal, (const goal_lineage*), ());
    MOCK_METHOD(void, pin_resolution, (const resolution_lineage*), ());
    MOCK_METHOD(void, trim, (), (override));
    MOCK_METHOD((const goal_lineage*), import_goal, (const goal_lineage*), ());
    MOCK_METHOD((const resolution_lineage*), import_resolution, (const resolution_lineage*), ());
    void pin(const goal_lineage* gl) override { pin_goal(gl); }
    void pin(const resolution_lineage* rl) override { pin_resolution(rl); }
    const goal_lineage* import(const goal_lineage* gl) override { return import_goal(gl); }
    const resolution_lineage* import(const resolution_lineage* rl) override {
        return import_resolution(rl);
    }
};

struct MockSolutionDetector : public i_solution_detector {
    MOCK_METHOD(bool, detect, (), (const, override));
};

struct MockConflictDetector : public i_conflict_detector {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (override));
};

struct MockUnitGoalDetector : public i_unit_goal_detector {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (const, override));
};

struct MockPushUnitGoal : public i_push_unit_goal {
    MOCK_METHOD(void, push, (const goal_lineage*), (override));
};

struct MockPopUnitGoal : public i_pop_unit_goal {
    MOCK_METHOD(const goal_lineage*, pop, (), (override));
};

struct MockCheckUnitGoalsEmpty : public i_check_unit_goals_empty {
    MOCK_METHOD(bool, empty, (), (const, override));
};

struct MockDecisionGenerator : public i_decision_generator {
    MOCK_METHOD(const resolution_lineage*, generate, (), (override));
};

struct MockEliminationGenerator : public i_elimination_generator {
    MOCK_METHOD(state_machine<const resolution_lineage*>, constrain, (const resolution_lineage*), (override));
};

struct MockEliminationRouter : public i_elimination_router {
    MOCK_METHOD(elimination_result, route, (const resolution_lineage*), (override));
};

struct MockResolver : public i_resolver {
    MOCK_METHOD(bool, resolve, (const resolution_lineage*), (override));
};

struct MockGetGoalCandidateRules : public i_get_goal_candidate_rules {
    MOCK_METHOD(i_rule_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_set&, get, (const goal_lineage*), (const, override));
};

struct SimTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 2;

    MockLineagePool lp;
    MockSolutionDetector solution_detector;
    MockConflictDetector conflict_detector;
    MockUnitGoalDetector unit_goal_detector;
    MockPushUnitGoal push_unit_goal;
    MockPopUnitGoal pop_unit_goal;
    MockCheckUnitGoalsEmpty check_unit_goals_empty;
    MockDecisionGenerator decision_generator;
    MockEliminationGenerator elimination_generator;
    MockEliminationRouter elimination_router;
    MockResolver resolver;
    MockGetGoalCandidateRules ggcr;
    sim simulation{
        kMaxResolutions,
        lp,
        solution_detector,
        conflict_detector,
        unit_goal_detector,
        push_unit_goal,
        pop_unit_goal,
        check_unit_goals_empty,
        decision_generator,
        elimination_generator,
        elimination_router,
        resolver,
        ggcr};

    expr goal_e{expr::var{0}};
    expr head{expr::var{1}};
    rule r{&head, {}};
    goal_lineage gl{nullptr, &goal_e};
    resolution_lineage rl{&gl, &r};
};

TEST_F(SimTest, RunReturnsSolvedWhenDetectorSaysSo) {
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(true));
    EXPECT_CALL(decision_generator, generate()).Times(0);
    EXPECT_EQ(simulation.run(), sim_termination::solved);
}

TEST_F(SimTest, RunReturnsDepthExceededWhenNoSolutionWithinLimit) {
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(check_unit_goals_empty, empty()).WillRepeatedly(Return(true));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));
    EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
}

TEST_F(SimTest, RunReturnsConflictedWhenResolverFails) {
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(check_unit_goals_empty, empty()).WillOnce(Return(true));
    EXPECT_CALL(decision_generator, generate()).WillOnce(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl)).WillOnce([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillOnce(Return(false));
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
}

TEST_F(SimTest, SetUpAndTearDownAreNoOps) {
    simulation.set_up();
    simulation.tear_down();
}
