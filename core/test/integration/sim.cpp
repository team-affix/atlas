// Integration: real sim orchestration slice (manifest wiring minus solver / random decisions).
// i_generate_decision is the only mocked collaborator — scripted resolutions keep runs deterministic.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include "infrastructure/sim.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/overlay_bind_map_factory.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/active_goals.hpp"
#include "infrastructure/goal_exprs.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/unit_goals.hpp"
#include "infrastructure/decision_memory.hpp"
#include "infrastructure/resolution_memory.hpp"
#include "infrastructure/deactivated_candidate_memory.hpp"
#include "infrastructure/candidate_translation_maps.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/var_sequencer.hpp"
#include "infrastructure/elimination_backlog.hpp"
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/get_resolution_rule.hpp"
#include "infrastructure/copier.hpp"
#include "infrastructure/conflict_detector.hpp"
#include "infrastructure/unit_goal_detector.hpp"
#include "infrastructure/solution_detector.hpp"
#include "infrastructure/goal_activator.hpp"
#include "infrastructure/goal_deactivator.hpp"
#include "infrastructure/candidate_activator.hpp"
#include "infrastructure/candidate_deactivator.hpp"
#include "infrastructure/elimination_router.hpp"
#include "infrastructure/get_unit_resolution.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "infrastructure/resolver.hpp"
#include "interfaces/i_generate_decision.hpp"
#include "interfaces/i_make_initial_goal_lineage.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_make_goal_lineage.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"
#include "interfaces/i_push_trail_frame.hpp"
#include "interfaces/i_pop_trail_frame.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "interfaces/i_bind_map.hpp"
#include "interfaces/i_clear_bindings.hpp"
#include "interfaces/i_bind_map_factory.hpp"
#include "interfaces/i_overlay_bind_map_factory.hpp"
#include "interfaces/i_unifier_factory.hpp"
#include "interfaces/i_make_goal_lineage.hpp"
#include "interfaces/i_pin_goal_lineage.hpp"
#include "interfaces/i_pin_resolution_lineage.hpp"
#include "interfaces/i_trim_unpinned_lineages.hpp"
#include "interfaces/i_import_goal_lineage.hpp"
#include "interfaces/i_import_resolution_lineage.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_erase_active_goal.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_iterate_active_goals.hpp"
#include "interfaces/i_active_goals_size.hpp"
#include "interfaces/i_clear_active_goals.hpp"
#include "interfaces/i_get_goal_expr.hpp"
#include "interfaces/i_set_goal_expr.hpp"
#include "interfaces/i_unset_goal_expr.hpp"
#include "interfaces/i_clear_goal_exprs.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_link_goal_candidate.hpp"
#include "interfaces/i_unlink_goal_candidate.hpp"
#include "interfaces/i_erase_goal_candidates.hpp"
#include "interfaces/i_clear_goal_candidate_rule_ids.hpp"
#include "interfaces/i_push_unit_goal.hpp"
#include "interfaces/i_pop_unit_goal.hpp"
#include "interfaces/i_clear_unit_goals.hpp"
#include "interfaces/i_record_decision.hpp"
#include "interfaces/i_clear_recorded_decisions.hpp"
#include "interfaces/i_get_decision_count.hpp"
#include "interfaces/i_derive_decision_lemma.hpp"
#include "interfaces/i_record_resolution.hpp"
#include "interfaces/i_clear_recorded_resolutions.hpp"
#include "interfaces/i_get_resolution_count.hpp"
#include "interfaces/i_derive_resolution_lemma.hpp"
#include "interfaces/i_deactivated_candidate_memory.hpp"
#include "interfaces/i_get_candidate_translation_map.hpp"
#include "interfaces/i_set_candidate_translation_map.hpp"
#include "interfaces/i_unset_candidate_translation_map.hpp"
#include "interfaces/i_clear_candidate_translation_maps.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_get_goal_db_rule_ids.hpp"
#include "interfaces/i_get_initial_goal_count.hpp"
#include "interfaces/i_get_initial_goal_expr.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "interfaces/i_import_expr.hpp"
#include "interfaces/i_get_expr_count.hpp"
#include "interfaces/i_var_sequencer.hpp"
#include "interfaces/i_insert_backlogged_elimination.hpp"
#include "interfaces/i_is_backlogged_elimination.hpp"
#include "interfaces/i_learn_avoidance.hpp"
#include "interfaces/i_try_add_mhu_head.hpp"
#include "interfaces/i_clear_mhu_heads.hpp"
#include "interfaces/i_elimination_generator.hpp"
#include "interfaces/i_get_resolution_rule.hpp"
#include "interfaces/i_copier.hpp"
#include "interfaces/i_conflict_detector.hpp"
#include "interfaces/i_detect_unit_goal.hpp"
#include "interfaces/i_solution_detector.hpp"
#include "interfaces/i_goal_activator.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_candidate_activator.hpp"
#include "interfaces/i_candidate_deactivator.hpp"
#include "interfaces/i_elimination_router.hpp"
#include "interfaces/i_get_unit_resolution.hpp"
#include "interfaces/i_activate_initial_goal.hpp"
#include "interfaces/i_resolver.hpp"
#include "interfaces/i_get_resolution_count.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"

using ::testing::Return;
using ::testing::UnorderedElementsAre;

struct MockGenerateDecision : public i_generate_decision {
    MOCK_METHOD(const resolution_lineage*, generate, (), (override));
};

namespace {

struct sim_early_wiring {
    trail trail_;
    bind_map bind_map_;
    bind_map_factory bind_map_factory_;
    overlay_bind_map_factory overlay_bind_map_factory_;
    unifier_factory unifier_factory_;
    lineage_pool lineage_pool_;
    active_goals active_goals_;
    goal_exprs goal_exprs_;
    goal_candidate_rules goal_candidate_rules_;
    unit_goals unit_goals_;
    decision_memory decision_memory_;
    resolution_memory resolution_memory_;
    deactivated_candidate_memory deactivated_candidate_memory_;
    candidate_translation_maps candidate_translation_maps_;

    sim_early_wiring(locator& loc, db& database, initial_goal_exprs& initial_goals)
        : trail_(),
          bind_map_(),
          bind_map_factory_(),
          overlay_bind_map_factory_(),
          unifier_factory_(),
          lineage_pool_(),
          active_goals_(),
          goal_exprs_(),
          goal_candidate_rules_(),
          unit_goals_(),
          decision_memory_(),
          resolution_memory_(),
          deactivated_candidate_memory_(),
          candidate_translation_maps_() {
        loc.bind_as<i_push_trail_frame, i_pop_trail_frame, i_log_to_current_trail_frame>(trail_);
        loc.bind_as<i_bind_map, i_clear_bindings>(bind_map_);
        loc.bind_as<i_bind_map_factory>(bind_map_factory_);
        loc.bind_as<i_overlay_bind_map_factory>(overlay_bind_map_factory_);
        loc.bind_as<i_unifier_factory>(unifier_factory_);
        loc.bind_as<i_make_goal_lineage, i_make_resolution_lineage, i_pin_goal_lineage,
            i_pin_resolution_lineage, i_trim_unpinned_lineages, i_import_goal_lineage,
            i_import_resolution_lineage>(lineage_pool_);
        loc.bind_as<i_insert_active_goal, i_erase_active_goal, i_is_active_goal,
            i_iterate_active_goals, i_active_goals_size, i_check_active_goals_empty,
            i_clear_active_goals>(active_goals_);
        loc.bind_as<i_get_goal_expr, i_set_goal_expr, i_unset_goal_expr, i_clear_goal_exprs>(
            goal_exprs_);
        loc.bind_as<i_get_goal_candidate_rule_ids, i_link_goal_candidate, i_unlink_goal_candidate,
            i_erase_goal_candidates, i_clear_goal_candidate_rule_ids>(goal_candidate_rules_);
        loc.bind_as<i_push_unit_goal, i_pop_unit_goal, i_clear_unit_goals>(unit_goals_);
        loc.bind_as<i_record_decision, i_clear_recorded_decisions, i_get_decision_count,
            i_derive_decision_lemma>(decision_memory_);
        loc.bind_as<i_record_resolution, i_clear_recorded_resolutions, i_get_resolution_count,
            i_derive_resolution_lemma>(resolution_memory_);
        loc.bind_as<i_deactivated_candidate_memory>(deactivated_candidate_memory_);
        loc.bind_as<i_get_candidate_translation_map, i_set_candidate_translation_map,
            i_unset_candidate_translation_map, i_clear_candidate_translation_maps>(
            candidate_translation_maps_);
        loc.bind_as<i_get_rule, i_get_goal_db_rule_ids>(database);
        loc.bind_as<i_get_initial_goal_count, i_get_initial_goal_expr>(initial_goals);
    }
};

struct sim_pool_wiring {
    expr_pool expr_pool_;
    var_sequencer var_sequencer_;
    elimination_backlog elimination_backlog_;

    sim_pool_wiring(locator& loc)
        : expr_pool_(loc), var_sequencer_(loc), elimination_backlog_(loc) {
        loc.bind_as<i_make_functor, i_make_var, i_import_expr, i_get_expr_count>(expr_pool_);
        loc.bind_as<i_var_sequencer>(var_sequencer_);
        loc.bind_as<i_insert_backlogged_elimination, i_is_backlogged_elimination>(
            elimination_backlog_);
    }
};

struct sim_elim_wiring {
    cdcl_elimination_generator cdcl_;
    mhu_elimination_generator mhu_;
    std::optional<joint_elimination_generator> joint_;

    sim_elim_wiring(locator& loc) : cdcl_(loc), mhu_(loc) {
        loc.bind_as<i_learn_avoidance>(cdcl_);
        loc.bind_as<i_try_add_mhu_head, i_clear_mhu_heads>(mhu_);
        joint_.emplace(loc);
        loc.bind_as<i_elimination_generator>(*joint_);
    }
};

struct sim_core_wiring {
    get_resolution_rule get_resolution_rule_;
    copier copier_;
    conflict_detector conflict_detector_;
    unit_goal_detector unit_goal_detector_;
    solution_detector solution_detector_;

    sim_core_wiring(locator& loc)
        : get_resolution_rule_(loc),
          copier_(loc),
          conflict_detector_(loc),
          unit_goal_detector_(loc),
          solution_detector_(loc) {
        loc.bind_as<i_get_resolution_rule>(get_resolution_rule_);
        loc.bind_as<i_copier>(copier_);
        loc.bind_as<i_conflict_detector>(conflict_detector_);
        loc.bind_as<i_detect_unit_goal>(unit_goal_detector_);
        loc.bind_as<i_solution_detector>(solution_detector_);
    }
};

struct sim_activator_wiring {
    goal_activator goal_activator_;
    goal_deactivator goal_deactivator_;
    candidate_activator candidate_activator_;
    candidate_deactivator candidate_deactivator_;

    sim_activator_wiring(locator& loc)
        : goal_activator_(loc),
          goal_deactivator_(loc),
          candidate_activator_(loc),
          candidate_deactivator_(loc) {
        loc.bind_as<i_goal_activator>(goal_activator_);
        loc.bind_as<i_goal_deactivator>(goal_deactivator_);
        loc.bind_as<i_candidate_activator>(candidate_activator_);
        loc.bind_as<i_candidate_deactivator>(candidate_deactivator_);
    }
};

struct sim_router_wiring {
    elimination_router elimination_router_;
    get_unit_resolution get_unit_resolution_;
    make_initial_goal_lineage make_initial_goal_lineage_;
    std::optional<initial_goal_activator> initial_goal_activator_;

    sim_router_wiring(locator& loc)
        : elimination_router_(loc),
          get_unit_resolution_(loc),
          make_initial_goal_lineage_(loc) {
        loc.bind_as<i_elimination_router>(elimination_router_);
        loc.bind_as<i_get_unit_resolution>(get_unit_resolution_);
        loc.bind_as<i_make_initial_goal_lineage>(make_initial_goal_lineage_);
        initial_goal_activator_.emplace(loc);
        loc.bind_as<i_activate_initial_goal>(*initial_goal_activator_);
    }
};

struct sim_stack {
    locator loc;
    db& database;
    initial_goal_exprs& initial_goals;
    sim_early_wiring early;
    sim_pool_wiring pools;
    sim_elim_wiring elims;
    sim_core_wiring core;
    sim_activator_wiring activators;
    sim_router_wiring routers;
    resolver resolver_;
    testing::NiceMock<MockGenerateDecision> decision_generator;

    sim_stack(db& database_in, initial_goal_exprs& initial_goals_in)
        : loc(),
          database(database_in),
          initial_goals(initial_goals_in),
          early(loc, database, initial_goals),
          pools(loc),
          elims(loc),
          core(loc),
          activators(loc),
          routers(loc),
          resolver_(loc) {
        loc.bind_as<i_generate_decision>(decision_generator);
        loc.bind_as<i_resolver>(resolver_);
    }
};

}  // namespace

struct SimIntegrationTest : public ::testing::Test {
    static constexpr size_t kDefaultMaxResolutions = 8;

    db database;
    initial_goal_exprs initial_goals;
    sim_stack stack{database, initial_goals};
};

TEST_F(SimIntegrationTest, RunWithNoInitialGoalsAndEmptyDbNeverGeneratesDecision) {
    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenInitialGoalHasNoDbCandidates) {
    expr goal{expr::functor{"f", {}}};
    initial_goals.push(&goal);

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenUnitFactAppliesToInitialGoal) {
    expr goal{expr::functor{"f", {}}};
    expr head{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenDbRuleHeadFailsToUnifyWithGoal) {
    expr goal{expr::functor{"f", {}}};
    expr head{expr::functor{"g", {}}};
    initial_goals.push(&goal);
    database.push(rule{&head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenOnlyOneOfTwoDbRulesUnifiesWithGoal) {
    expr goal{expr::functor{"f", {}}};
    expr matching_head{expr::functor{"f", {}}};
    expr mismatching_head{expr::functor{"g", {}}};
    initial_goals.push(&goal);
    database.push(rule{&matching_head, {}});
    database.push(rule{&mismatching_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterSingleDecisionWithTwoMatchingFacts) {
    expr goal{expr::functor{"f", {}}};
    expr head0{expr::functor{"f", {}}};
    expr head1{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    const goal_lineage* gl = stack.loc.locate<i_make_initial_goal_lineage>().make(0);
    const resolution_lineage* chosen =
        stack.loc.locate<i_make_resolution_lineage>().make_resolution_lineage(gl, rule_id{1});
    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(chosen));

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedViaClauseThenBodyFactWithoutDecisions) {
    expr goal{expr::functor{"f", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_body{expr::functor{"g", {}}};
    expr g_head{expr::functor{"g", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head, {&g_body}});
    database.push(rule{&g_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenSecondInitialGoalHasNoCandidates) {
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head{expr::functor{"f", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenTwoInitialGoalsEachHaveMatchingFacts) {
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_head{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head, {}});
    database.push(rule{&g_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterOneDecisionWhenInitialGoalFHasTwoMatchingFacts) {
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head, {}});

    const goal_lineage* gl_f = stack.loc.locate<i_make_initial_goal_lineage>().make(0);
    const resolution_lineage* chosen =
        stack.loc.locate<i_make_resolution_lineage>().make_resolution_lineage(gl_f, rule_id{0});
    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(chosen));

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenSecondInitialGoalLacksCandidatesDespiteTwoFFacts) {
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenRuleZeroIsBackloggedBeforeRun) {
    expr goal{expr::functor{"f", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();
    i_insert_backlogged_elimination& insert_backlogged_elimination =
        stack.loc.locate<i_insert_backlogged_elimination>();
    i_derive_resolution_lemma& derive_resolution_lemma =
        stack.loc.locate<i_derive_resolution_lemma>();

    const goal_lineage* gl = make_initial_goal_lineage.make(0);
    const resolution_lineage* rl0 =
        make_resolution_lineage.make_resolution_lineage(gl, rule_id{0});
    const resolution_lineage* rl1 =
        make_resolution_lineage.make_resolution_lineage(gl, rule_id{1});

    insert_backlogged_elimination.insert_backlogged_elimination(rl0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl1));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterCdclAvoidanceForcesG1RuleThree) {
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head2{expr::functor{"g", {}}};
    expr g_head3{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head2, {}});
    database.push(rule{&g_head3, {}});

    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();
    i_learn_avoidance& learn_avoidance = stack.loc.locate<i_learn_avoidance>();
    i_derive_resolution_lemma& derive_resolution_lemma =
        stack.loc.locate<i_derive_resolution_lemma>();

    const goal_lineage* gl0 = make_initial_goal_lineage.make(0);
    const goal_lineage* gl1 = make_initial_goal_lineage.make(1);
    const resolution_lineage* rl_g0_0 =
        make_resolution_lineage.make_resolution_lineage(gl0, rule_id{0});
    const resolution_lineage* rl_g1_2 =
        make_resolution_lineage.make_resolution_lineage(gl1, rule_id{2});
    const resolution_lineage* rl_g1_3 =
        make_resolution_lineage.make_resolution_lineage(gl1, rule_id{3});

    learn_avoidance.learn(lemma{{rl_g0_0, rl_g1_2}});

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_g0_0));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_g0_0, rl_g1_3));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedOnRecursiveClauseTreeWithoutDecisions) {
    expr goal_f{expr::functor{"f", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_body{expr::functor{"g", {}}};
    expr h_body{expr::functor{"h", {}}};
    expr g_head{expr::functor{"g", {}}};
    expr h_head{expr::functor{"h", {}}};
    expr i_body{expr::functor{"i", {}}};
    expr j_body{expr::functor{"j", {}}};
    expr i_head{expr::functor{"i", {}}};
    expr j_head{expr::functor{"j", {}}};
    initial_goals.push(&goal_f);
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {&i_body, &j_body}});
    database.push(rule{&h_head, {&i_body, &j_body}});
    database.push(rule{&i_head, {}});
    database.push(rule{&j_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    sim simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}
