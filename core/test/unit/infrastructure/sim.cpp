// Simulation loop: solution/conflict/depth termination, elimination routing, and
// resolution stepping. set_up pushes a trail frame and activates initial goals;
// tear_down is a no-op. run() must honor max_resolutions and short-circuit on
// solution_detector.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/sim.hpp"
#include "../../../core/hpp/infrastructure/rule_id_set.hpp"
#include "../../../core/hpp/interfaces/i_make_resolution_lineage.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_db_rule_ids.hpp"
#include "../../../core/hpp/interfaces/i_candidate_activator.hpp"
#include "../../../core/hpp/interfaces/i_solution_detector.hpp"
#include "../../../core/hpp/interfaces/i_conflict_detector.hpp"
#include "../../../core/hpp/interfaces/i_detect_unit_goal.hpp"
#include "../../../core/hpp/interfaces/i_push_unit_goal.hpp"
#include "../../../core/hpp/interfaces/i_pop_unit_goal.hpp"
#include "../../../core/hpp/interfaces/i_generate_decision.hpp"
#include "../../../core/hpp/interfaces/i_elimination_generator.hpp"
#include "../../../core/hpp/interfaces/i_elimination_router.hpp"
#include "../../../core/hpp/interfaces/i_resolver.hpp"
#include "../../../core/hpp/interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "../../../core/hpp/interfaces/i_activate_initial_goal.hpp"
#include "../../../core/hpp/interfaces/i_get_initial_goal_count.hpp"
#include "../../../core/hpp/interfaces/i_make_initial_goal_lineage.hpp"
#include "../../../core/hpp/utility/i_trail.hpp"
#include "../../../core/hpp/value_objects/elimination_result.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

namespace {

state_machine<const resolution_lineage*> empty_eliminations() {
    co_return;
}

}  // namespace

struct MockMakeResolutionLineage : public i_make_resolution_lineage {
    MOCK_METHOD((const resolution_lineage*), make_resolution_lineage,
        (const goal_lineage*, rule_id), (override));
};

struct MockGetGoalDbRuleIds : public i_get_goal_db_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
};

struct MockCandidateActivator : public i_candidate_activator {
    MOCK_METHOD(void, activate, (const resolution_lineage*), (override));
};

struct MockSolutionDetector : public i_solution_detector {
    MOCK_METHOD(bool, detect, (), (const, override));
};

struct MockConflictDetector : public i_conflict_detector {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (const, override));
};

struct MockUnitGoalDetector : public i_detect_unit_goal {
    MOCK_METHOD(bool, detect, (const goal_lineage*), (const, override));
};

struct MockPushUnitGoal : public i_push_unit_goal {
    MOCK_METHOD(void, push, (const goal_lineage*), (override));
};

struct MockPopUnitGoal : public i_pop_unit_goal {
    MOCK_METHOD(std::optional<const goal_lineage*>, pop, (), (override));
};

struct MockGenerateDecision : public i_generate_decision {
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

struct MockGetGoalCandidateRuleIds : public i_get_goal_candidate_rule_ids {
    MOCK_METHOD(i_rule_id_set&, get, (const goal_lineage*), (override));
    MOCK_METHOD(const i_rule_id_set&, get, (const goal_lineage*), (const, override));
};

struct MockTrail : public i_trail {
    MOCK_METHOD(void, push, (), (override));
    MOCK_METHOD(void, pop, (), (override));
    MOCK_METHOD(void, log, (std::unique_ptr<i_backtrackable>), (override));
};

struct MockGetInitialGoalCount : public i_get_initial_goal_count {
    MOCK_METHOD(size_t, count, (), (const, override));
};

struct MockMakeInitialGoalLineage : public i_make_initial_goal_lineage {
    MOCK_METHOD(const goal_lineage*, make, (subgoal_id), (override));
};

struct MockActivateInitialGoal : public i_activate_initial_goal {
    MOCK_METHOD(void, activate_initial_goal, (subgoal_id), (override));
};

struct SimTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 2;

    MockTrail trail;
    MockGetInitialGoalCount get_initial_goal_count;
    MockActivateInitialGoal activate_initial_goal;
    MockMakeInitialGoalLineage make_initial_goal_lineage;
    MockGetGoalDbRuleIds get_goal_db_rule_ids;
    MockMakeResolutionLineage lp;
    MockCandidateActivator candidate_activator;
    MockSolutionDetector solution_detector;
    MockConflictDetector conflict_detector;
    MockUnitGoalDetector unit_goal_detector;
    MockPushUnitGoal push_unit_goal;
    MockPopUnitGoal pop_unit_goal;
    MockGenerateDecision decision_generator;
    MockEliminationGenerator elimination_generator;
    MockEliminationRouter elimination_router;
    MockResolver resolver;
    MockGetGoalCandidateRuleIds get_goal_candidate_rule_ids;
    sim simulation{
        kMaxResolutions,
        trail,
        get_initial_goal_count,
        activate_initial_goal,
        make_initial_goal_lineage,
        get_goal_db_rule_ids,
        lp,
        candidate_activator,
        solution_detector,
        conflict_detector,
        unit_goal_detector,
        push_unit_goal,
        pop_unit_goal,
        decision_generator,
        elimination_generator,
        elimination_router,
        resolver,
        get_goal_candidate_rule_ids};

    expr goal_e{expr::var{0}};
    expr head{expr::var{1}};
    rule r{&head, {}};
    goal_lineage gl{nullptr, 0};
    resolution_lineage rl{&gl, 0};
    rule_id_set db_rules;
};

TEST_F(SimTest, RunReturnsSolvedWhenDetectorSaysSo) {
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(true));
    EXPECT_CALL(decision_generator, generate()).Times(0);
    EXPECT_EQ(simulation.run(), sim_termination::solved);
}

TEST_F(SimTest, RunReturnsDepthExceededWhenNoSolutionWithinLimit) {
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));
    EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
}

TEST_F(SimTest, RunReturnsConflictedWhenResolverFails) {
    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillOnce(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl)).WillOnce([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillOnce(Return(false));
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
}

TEST_F(SimTest, SetUpPushesTrailAndActivatesEachInitialGoal) {
    EXPECT_CALL(trail, push()).Times(1);
    EXPECT_CALL(get_initial_goal_count, count()).WillRepeatedly(Return(2));
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
    EXPECT_CALL(activate_initial_goal, activate_initial_goal(0)).Times(1);
    EXPECT_CALL(activate_initial_goal, activate_initial_goal(1)).Times(1);
    EXPECT_CALL(make_initial_goal_lineage, make(0)).WillOnce(Return(&gl0));
    EXPECT_CALL(make_initial_goal_lineage, make(1)).WillOnce(Return(&gl1));
    EXPECT_CALL(get_goal_db_rule_ids, get(&gl0)).WillOnce(ReturnRef(db_rules));
    EXPECT_CALL(get_goal_db_rule_ids, get(&gl1)).WillOnce(ReturnRef(db_rules));
    simulation.set_up();
    simulation.tear_down();
}
