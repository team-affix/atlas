// Simulation loop: solution/conflict/depth termination, elimination routing, and
// resolution stepping. set_up pushes a trail frame and activates initial goals;
// tear_down pops the trail and clears non-backtracked stores. run() must honor
// max_resolutions and short-circuit on solution_detector.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include "infrastructure/sim.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_get_goal_db_rule_ids.hpp"
#include "interfaces/i_candidate_activator.hpp"
#include "interfaces/i_solution_detector.hpp"
#include "interfaces/i_conflict_detector.hpp"
#include "interfaces/i_detect_unit_goal.hpp"
#include "interfaces/i_push_unit_goal.hpp"
#include "interfaces/i_pop_unit_goal.hpp"
#include "interfaces/i_generate_decision.hpp"
#include "interfaces/i_elimination_generator.hpp"
#include "interfaces/i_elimination_router.hpp"
#include "interfaces/i_resolver.hpp"
#include "interfaces/i_get_unit_resolution.hpp"
#include "interfaces/i_record_decision.hpp"
#include "interfaces/i_record_resolution.hpp"
#include "interfaces/i_activate_initial_goal.hpp"
#include "interfaces/i_get_initial_goal_count.hpp"
#include "interfaces/i_make_initial_goal_lineage.hpp"
#include "interfaces/i_push_trail_frame.hpp"
#include "interfaces/i_pop_trail_frame.hpp"
#include "interfaces/i_clear_bindings.hpp"
#include "value_objects/elimination_result.hpp"

using ::testing::Return;
using ::testing::ReturnRef;

namespace {

state_machine<const resolution_lineage*> empty_eliminations() {
    co_return;
}

state_machine<const resolution_lineage*> single_elimination(const resolution_lineage* elim) {
    co_yield elim;
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

struct MockGetUnitResolution : public i_get_unit_resolution {
    MOCK_METHOD(const resolution_lineage*, get, (const goal_lineage*), (override));
};

struct MockPushTrailFrame : public i_push_trail_frame {
    MOCK_METHOD(void, push, (), (override));
};

struct MockPopTrailFrame : public i_pop_trail_frame {
    MOCK_METHOD(void, pop, (), (override));
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

struct MockRecordDecision : public i_record_decision {
    MOCK_METHOD(void, record_decision, (const resolution_lineage*), (override));
};

struct MockRecordResolution : public i_record_resolution {
    MOCK_METHOD(void, record_resolution, (const resolution_lineage*), (override));
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

struct MockDeactivatedCandidateMemory : public i_deactivated_candidate_memory {
    MOCK_METHOD(void, insert, (const resolution_lineage*), (override));
    MOCK_METHOD(void, clear, (), (override));
    MOCK_METHOD(bool, contains, (const resolution_lineage*), (const, override));
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

struct MockClearCandidateTranslationMaps : public i_clear_candidate_translation_maps {
    MOCK_METHOD(void, clear_candidate_translation_maps, (), (override));
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

struct SimTest : public ::testing::Test {
    static constexpr size_t kMaxResolutions = 2;

    MockPushTrailFrame push_trail_frame;
    MockPopTrailFrame pop_trail_frame;
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
    MockGetUnitResolution get_unit_resolution;
    testing::NiceMock<MockRecordDecision> record_decision;
    testing::NiceMock<MockRecordResolution> record_resolution;
    testing::NiceMock<MockClearUnitGoals> clear_unit_goals;
    testing::NiceMock<MockClearRecordedDecisions> clear_recorded_decisions;
    testing::NiceMock<MockClearRecordedResolutions> clear_recorded_resolutions;
    testing::NiceMock<MockDeactivatedCandidateMemory> deactivated_candidate_memory;
    testing::NiceMock<MockClearGoalCandidateRuleIds> clear_goal_candidate_rule_ids;
    testing::NiceMock<MockClearGoalExprs> clear_goal_exprs;
    testing::NiceMock<MockClearActiveGoals> clear_active_goals;
    testing::NiceMock<MockClearCandidateTranslationMaps> clear_candidate_translation_maps;
    testing::NiceMock<MockClearMhuHeads> clear_mhu_heads;
    testing::NiceMock<MockClearBindings> clear_bindings;
    testing::NiceMock<MockTrimUnpinnedLineages> trim_unpinned_lineages;

    locator loc;

    void bind_sim_deps() {
        loc.bind_as<i_push_trail_frame>(push_trail_frame);
        loc.bind_as<i_pop_trail_frame>(pop_trail_frame);
        loc.bind_as<i_get_initial_goal_count>(get_initial_goal_count);
        loc.bind_as<i_activate_initial_goal>(activate_initial_goal);
        loc.bind_as<i_make_initial_goal_lineage>(make_initial_goal_lineage);
        loc.bind_as<i_get_goal_db_rule_ids>(get_goal_db_rule_ids);
        loc.bind_as<i_make_resolution_lineage>(lp);
        loc.bind_as<i_candidate_activator>(candidate_activator);
        loc.bind_as<i_solution_detector>(solution_detector);
        loc.bind_as<i_conflict_detector>(conflict_detector);
        loc.bind_as<i_detect_unit_goal>(unit_goal_detector);
        loc.bind_as<i_push_unit_goal>(push_unit_goal);
        loc.bind_as<i_pop_unit_goal>(pop_unit_goal);
        loc.bind_as<i_generate_decision>(decision_generator);
        loc.bind_as<i_elimination_generator>(elimination_generator);
        loc.bind_as<i_elimination_router>(elimination_router);
        loc.bind_as<i_resolver>(resolver);
        loc.bind_as<i_get_unit_resolution>(get_unit_resolution);
        loc.bind_as<i_record_decision>(record_decision);
        loc.bind_as<i_record_resolution>(record_resolution);
        loc.bind_as<i_clear_unit_goals>(clear_unit_goals);
        loc.bind_as<i_clear_recorded_decisions>(clear_recorded_decisions);
        loc.bind_as<i_clear_recorded_resolutions>(clear_recorded_resolutions);
        loc.bind_as<i_deactivated_candidate_memory>(deactivated_candidate_memory);
        loc.bind_as<i_clear_goal_candidate_rule_ids>(clear_goal_candidate_rule_ids);
        loc.bind_as<i_clear_goal_exprs>(clear_goal_exprs);
        loc.bind_as<i_clear_active_goals>(clear_active_goals);
        loc.bind_as<i_clear_candidate_translation_maps>(clear_candidate_translation_maps);
        loc.bind_as<i_clear_mhu_heads>(clear_mhu_heads);
        loc.bind_as<i_clear_bindings>(clear_bindings);
        loc.bind_as<i_trim_unpinned_lineages>(trim_unpinned_lineages);
    }

    sim make_sim(size_t max_resolutions = kMaxResolutions) {
        return sim{loc, max_resolutions};
    }

    sim simulation;

    SimTest() : simulation(init_simulation()) {}

    sim init_simulation() {
        bind_sim_deps();
        return sim{loc, kMaxResolutions};
    }

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

TEST_F(SimTest, TearDownPopsTrailAndClearsNonBacktrackedStores) {
    EXPECT_CALL(pop_trail_frame, pop()).Times(1);
    EXPECT_CALL(clear_unit_goals, clear()).Times(1);
    EXPECT_CALL(clear_recorded_decisions, clear_recorded_decisions()).Times(1);
    EXPECT_CALL(clear_recorded_resolutions, clear_recorded_resolutions()).Times(1);
    EXPECT_CALL(deactivated_candidate_memory, clear()).Times(1);
    EXPECT_CALL(clear_goal_candidate_rule_ids, clear_goal_candidate_rule_ids()).Times(1);
    EXPECT_CALL(clear_goal_exprs, clear_goal_exprs()).Times(1);
    EXPECT_CALL(clear_active_goals, clear_active_goals()).Times(1);
    EXPECT_CALL(clear_candidate_translation_maps, clear_candidate_translation_maps()).Times(1);
    EXPECT_CALL(clear_mhu_heads, clear_mhu_heads()).Times(1);
    EXPECT_CALL(clear_bindings, clear_bindings()).Times(1);
    EXPECT_CALL(trim_unpinned_lineages, trim()).Times(1);

    simulation.tear_down();
}

TEST_F(SimTest, SetUpPushesTrailAndActivatesEachInitialGoal) {
    EXPECT_CALL(push_trail_frame, push()).Times(1);
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

TEST_F(SimTest, SetUpActivatesDbRuleCandidates) {
    static constexpr rule_id kDbRule = 3;
    db_rules.insert(kDbRule);
    goal_lineage gl0{nullptr, 0};
    resolution_lineage db_rl{&gl0, kDbRule};

    EXPECT_CALL(push_trail_frame, push()).Times(1);
    EXPECT_CALL(get_initial_goal_count, count()).WillRepeatedly(Return(1));
    EXPECT_CALL(activate_initial_goal, activate_initial_goal(0)).Times(1);
    EXPECT_CALL(make_initial_goal_lineage, make(0)).WillOnce(Return(&gl0));
    EXPECT_CALL(get_goal_db_rule_ids, get(&gl0)).WillOnce(ReturnRef(db_rules));
    EXPECT_CALL(lp, make_resolution_lineage(&gl0, kDbRule)).WillOnce(Return(&db_rl));
    EXPECT_CALL(candidate_activator, activate(&db_rl)).Times(1);

    simulation.set_up();
}

TEST_F(SimTest, RunRoutesEliminationBeforeResolve) {
    resolution_lineage elim_rl{&gl, 1};

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

    EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
}

TEST_F(SimTest, RunReturnsConflictedWhenEliminationParentConflicts) {
    resolution_lineage elim_rl{&gl, 1};

    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillOnce(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillOnce(Return(&rl));
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillOnce([&] { return single_elimination(&elim_rl); });
    EXPECT_CALL(elimination_router, route(&elim_rl)).Times(1);
    EXPECT_CALL(conflict_detector, detect(&gl)).WillOnce(Return(true));
    EXPECT_CALL(resolver, resolve).Times(0);

    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
}

TEST_F(SimTest, RunPushesUnitGoalWhenEliminationParentIsUnit) {
    resolution_lineage elim_rl{&gl, 1};

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

    EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
}

TEST_F(SimTest, RecordsDecisionWhenGeneratorChoosesResolution) {
    EXPECT_CALL(solution_detector, detect()).WillRepeatedly(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillRepeatedly(Return(std::nullopt));
    EXPECT_CALL(decision_generator, generate()).WillRepeatedly(Return(&rl));
    EXPECT_CALL(record_decision, record_decision(&rl)).Times(2);
    EXPECT_CALL(record_resolution, record_resolution(&rl)).Times(2);
    EXPECT_CALL(elimination_generator, constrain(&rl))
        .WillRepeatedly([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&rl)).WillRepeatedly(Return(true));

    EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
}

TEST_F(SimTest, RunUsesPoppedUnitGoalForNextResolution) {
    resolution_lineage unit_rl{&gl, 5};

    sim one_resolution{make_sim(1)};

    EXPECT_CALL(solution_detector, detect()).WillOnce(Return(false));
    EXPECT_CALL(pop_unit_goal, pop()).WillOnce(Return(&gl));
    EXPECT_CALL(get_unit_resolution, get(&gl)).WillOnce(Return(&unit_rl));
    EXPECT_CALL(decision_generator, generate()).Times(0);
    EXPECT_CALL(record_decision, record_decision).Times(0);
    EXPECT_CALL(record_resolution, record_resolution(&unit_rl)).Times(1);
    EXPECT_CALL(elimination_generator, constrain(&unit_rl))
        .WillOnce([] { return empty_eliminations(); });
    EXPECT_CALL(resolver, resolve(&unit_rl)).WillOnce(Return(true));

    EXPECT_EQ(one_resolution.run(), sim_termination::depth_exceeded);
}
