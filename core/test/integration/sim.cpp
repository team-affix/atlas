// Integration: real sim orchestration slice (manifest wiring minus solver / random decisions).
// i_generate_decision is the only mocked collaborator — scripted resolutions keep runs deterministic.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <optional>
#include <string>
#include <vector>
#include "infrastructure/set_up_sim.hpp"
#include "infrastructure/tear_down_sim.hpp"
#include "infrastructure/run_sim.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/ra_active_goals.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/goal_exprs.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/unit_goals.hpp"
#include "infrastructure/decision_memory.hpp"
#include "infrastructure/resolution_memory.hpp"
#include "infrastructure/candidate_translation_maps.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/cdcl_sequencer.hpp"
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
#include "infrastructure/goal_candidates_activator.hpp"
#include "infrastructure/goal_candidates_deactivator.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "infrastructure/resolver.hpp"
#include "interfaces/i_activate_goal_candidates.hpp"
#include "interfaces/i_deactivate_goal_candidates.hpp"
#include "interfaces/i_activate_subgoals_and_candidates.hpp"
#include "interfaces/i_activate_initial_goals_and_candidates.hpp"
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
#include "interfaces/i_unifier_factory.hpp"
#include "interfaces/i_pin_goal_lineage.hpp"
#include "interfaces/i_pin_resolution_lineage.hpp"
#include "interfaces/i_trim_unpinned_lineages.hpp"
#include "interfaces/i_import_goal_lineage.hpp"
#include "interfaces/i_import_resolution_lineage.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_erase_active_goal.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_random_access.hpp"
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
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"

using ::testing::IsEmpty;
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
    unifier_factory unifier_factory_;
    lineage_pool lineage_pool_;
    ra_rule_id_set_factory ra_rule_id_set_factory_;
    ra_active_goals ra_active_goals_;
    goal_exprs goal_exprs_;
    goal_candidate_rules goal_candidate_rules_;
    unit_goals unit_goals_;
    decision_memory decision_memory_;
    resolution_memory resolution_memory_;
    candidate_translation_maps candidate_translation_maps_;

    sim_early_wiring(locator& loc, db& database, initial_goal_exprs& initial_goals)
        : trail_(),
          bind_map_(),
          bind_map_factory_(),
          unifier_factory_(),
          lineage_pool_(),
          ra_rule_id_set_factory_(),
          ra_active_goals_(),
          goal_exprs_(),
          goal_candidate_rules_(ra_rule_id_set_factory_),
          unit_goals_(),
          decision_memory_(),
          resolution_memory_(),
          candidate_translation_maps_() {
        loc.bind_as<i_push_trail_frame, i_pop_trail_frame, i_log_to_current_trail_frame>(trail_);
        loc.bind_as<i_bind_map, i_clear_bindings>(bind_map_);
        loc.bind_as<i_bind_map_factory>(bind_map_factory_);
        loc.bind_as<i_unifier_factory>(unifier_factory_);
        loc.bind_as<i_make_goal_lineage, i_make_resolution_lineage, i_pin_goal_lineage,
            i_pin_resolution_lineage, i_trim_unpinned_lineages, i_import_goal_lineage,
            i_import_resolution_lineage>(lineage_pool_);
        loc.bind_as<i_insert_active_goal, i_erase_active_goal, i_is_active_goal,
            i_active_goals_size, i_check_active_goals_empty, i_clear_active_goals,
            i_random_access<const goal_lineage*>>(ra_active_goals_);
        loc.bind_as<i_get_goal_expr, i_set_goal_expr, i_unset_goal_expr, i_clear_goal_exprs>(
            goal_exprs_);
        loc.bind_as<i_get_goal_candidate_rule_ids, i_insert_goal_candidates, i_link_goal_candidate,
            i_unlink_goal_candidate, i_erase_goal_candidates, i_clear_goal_candidate_rule_ids>(
            goal_candidate_rules_);
        loc.bind_as<i_push_unit_goal, i_pop_unit_goal, i_clear_unit_goals>(unit_goals_);
        loc.bind_as<i_record_decision, i_clear_recorded_decisions, i_get_decision_count,
            i_derive_decision_lemma>(decision_memory_);
        loc.bind_as<i_record_resolution, i_clear_recorded_resolutions, i_get_resolution_count,
            i_derive_resolution_lemma>(resolution_memory_);
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
    cdcl_sequencer cdcl_sequencer_;
    elimination_backlog elimination_backlog_;

    sim_pool_wiring(locator& loc)
        : expr_pool_(loc),
          var_sequencer_(loc, 0),
          cdcl_sequencer_(loc),
          elimination_backlog_(loc) {
        loc.bind_as<i_make_functor, i_make_var, i_import_expr, i_get_expr_count>(expr_pool_);
        loc.bind_as<i_var_sequencer>(var_sequencer_);
        loc.bind_as<i_cdcl_sequencer>(cdcl_sequencer_);
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

struct sim_orch_wiring {
    goal_candidates_activator goal_candidates_activator_;
    goal_candidates_deactivator goal_candidates_deactivator_;
    std::optional<subgoals_activator> subgoals_activator_;
    std::optional<initial_goals_activator> initial_goals_activator_;
    std::optional<resolver> resolver_;

    sim_orch_wiring(locator& loc)
        : goal_candidates_activator_(loc),
          goal_candidates_deactivator_(loc) {
        loc.bind_as<i_activate_goal_candidates>(goal_candidates_activator_);
        loc.bind_as<i_deactivate_goal_candidates>(goal_candidates_deactivator_);
        subgoals_activator_.emplace(loc);
        loc.bind_as<i_activate_subgoals_and_candidates>(*subgoals_activator_);
        initial_goals_activator_.emplace(loc);
        loc.bind_as<i_activate_initial_goals_and_candidates>(*initial_goals_activator_);
        resolver_.emplace(loc);
        loc.bind_as<i_resolver>(*resolver_);
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
    sim_orch_wiring orch;
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
          orch(loc) {
        loc.bind_as<i_generate_decision>(decision_generator);
    }
};

const expr* make_suc_n(i_make_functor& make_functor, const expr* zero, int n) {
    const expr* cur = zero;
    for (int i = 0; i < n; ++i)
        cur = make_functor.make("suc", {cur});
    return cur;
}

// chain_i(X) :- chain_{i+1}(X) for i in [0, depth-2]; chain_{depth-1}(ground_atom) fact.
void chain_clause_db(db& database, std::vector<expr>& storage, const char* prefix, size_t depth,
    const expr* ground_atom) {
    storage.reserve(depth * 3);
    for (size_t i = 0; i + 1 < depth; ++i) {
        storage.emplace_back(expr::var{0});
        const size_t x_idx = storage.size() - 1;
        storage.emplace_back(
            expr::functor{std::string(prefix) + std::to_string(i), {&storage[x_idx]}});
        storage.emplace_back(
            expr::functor{std::string(prefix) + std::to_string(i + 1), {&storage[x_idx]}});
        database.push(rule{&storage[storage.size() - 2], {&storage[storage.size() - 1]}});
    }
    storage.emplace_back(
        expr::functor{std::string(prefix) + std::to_string(depth - 1), {ground_atom}});
    database.push(rule{&storage.back(), {}});
}

}  // namespace

struct simulation {
    set_up_sim set_up_sim_;
    run_sim run_sim_;
    tear_down_sim tear_down_sim_;

    simulation(locator& loc, size_t max_resolutions)
        : set_up_sim_(loc), run_sim_(loc, max_resolutions), tear_down_sim_(loc) {}

    void set_up() { set_up_sim_.set_up(); }
    sim_termination run() { return run_sim_.run(); }
    void tear_down() { tear_down_sim_.tear_down(); }
};

struct SimIntegrationTest : public ::testing::Test {
    static constexpr size_t kDefaultMaxResolutions = 8;

    db database;
    initial_goal_exprs initial_goals;
    sim_stack stack{database, initial_goals};
};

TEST_F(SimIntegrationTest, RunWithNoInitialGoalsAndEmptyDbNeverGeneratesDecision) {
    /*
     * initial goals: (none)
     * database: (empty)
     */
    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenInitialGoalHasNoDbCandidates) {
    /*
     * initial goals:
     *   f.
     * database: (empty)
     */
    expr goal{expr::functor{"f", {}}};
    initial_goals.push(&goal);

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenUnitFactAppliesToInitialGoal) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f.
     */
    expr goal{expr::functor{"f", {}}};
    expr head{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenDbRuleHeadFailsToUnifyWithGoal) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: g.
     */
    expr goal{expr::functor{"f", {}}};
    expr head{expr::functor{"g", {}}};
    initial_goals.push(&goal);
    database.push(rule{&head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenOnlyOneOfTwoDbRulesUnifiesWithGoal) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f.
     *   1: g.
     */
    expr goal{expr::functor{"f", {}}};
    expr matching_head{expr::functor{"f", {}}};
    expr mismatching_head{expr::functor{"g", {}}};
    initial_goals.push(&goal);
    database.push(rule{&matching_head, {}});
    database.push(rule{&mismatching_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterSingleDecisionWithTwoMatchingFacts) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f.
     *   1: f.
     * setup: decision picks rule 1
     */
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

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedViaClauseThenBodyFactWithoutDecisions) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f :- g.
     *   1: g.
     */
    expr goal{expr::functor{"f", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_body{expr::functor{"g", {}}};
    expr g_head{expr::functor{"g", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head, {&g_body}});
    database.push(rule{&g_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenSecondInitialGoalHasNoCandidates) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head{expr::functor{"f", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenTwoInitialGoalsEachHaveMatchingFacts) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: g.
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_head{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head, {}});
    database.push(rule{&g_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterOneDecisionWhenInitialGoalFHasTwoMatchingFacts) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     *   2: g.
     * setup: decision picks rule 0 for goal f
     */
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

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenSecondInitialGoalLacksCandidatesDespiteTwoFFacts) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenRuleZeroIsBackloggedBeforeRun) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f.
     *   1: f.
     * setup: rule 0 backlogged before run; expects resolution via rule 1 only
     */
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

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl1));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterCdclAvoidanceForcesG1RuleThree) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     *   2: g.
     *   3: g.
     * setup: CDCL avoidance {{goal f, rule 0}, {goal g, rule 2}}; decision picks f rule 0
     */
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

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_g0_0));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_g0_0, rl_g1_3));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedOnRecursiveClauseTreeWithoutDecisions) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f :- g, h.
     *   1: g :- i, j.
     *   2: h :- i, j.
     *   3: i.
     *   4: j.
     */
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

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterOneDecisionOnInitialGoalKWithTwoFacts) {
    /*
     * initial goals:
     *   f.
     *   g.
     *   k.
     * database:
     *   0: f :- g, h.
     *   1: g :- i, j.
     *   2: h :- i, j.
     *   3: i.
     *   4: j.
     *   5: k.
     *   6: k.
     * setup: decision picks rule 5 for goal k
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr goal_k{expr::functor{"k", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_body{expr::functor{"g", {}}};
    expr h_body{expr::functor{"h", {}}};
    expr g_head{expr::functor{"g", {}}};
    expr h_head{expr::functor{"h", {}}};
    expr i_body{expr::functor{"i", {}}};
    expr j_body{expr::functor{"j", {}}};
    expr k_head0{expr::functor{"k", {}}};
    expr k_head1{expr::functor{"k", {}}};
    expr i_head{expr::functor{"i", {}}};
    expr j_head{expr::functor{"j", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    initial_goals.push(&goal_k);
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {&i_body, &j_body}});
    database.push(rule{&h_head, {&i_body, &j_body}});
    database.push(rule{&i_head, {}});
    database.push(rule{&j_head, {}});
    database.push(rule{&k_head0, {}});
    database.push(rule{&k_head1, {}});

    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();

    const goal_lineage* gl_k = make_initial_goal_lineage.make(2);
    const resolution_lineage* rl_k_first =
        make_resolution_lineage.make_resolution_lineage(gl_k, rule_id{5});

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_k_first));

    static constexpr size_t kMaxResolutions = 32;
    simulation simulation{stack.loc, kMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAndBindsVarsViaMhuWhenFactUnifiesGoal) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(abc, 123).
     */
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head{expr::functor{"f", {&abc, &_123}}};
    database.push(rule{&head, {}});

    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const uint32_t idx_a = seq.next();
    const uint32_t idx_b = seq.next();
    const expr* var_a = make_var.make(idx_a);
    const expr* var_b = make_var.make(idx_b);
    initial_goals.push(make_functor.make("f", {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf(var_b)->content);
    EXPECT_EQ(whnf_a.name, "abc");
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.name, "123");
    EXPECT_TRUE(whnf_b.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedViaClauseBodyFactsBindingVarsWithoutDecisions) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(C, D) :- g(C), h(D).
     *   1: g(abc).
     *   2: h(123).
     */
    expr rule_var_a{expr::var{0}};
    expr rule_var_b{expr::var{1}};
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr f_head{expr::functor{"f", {&rule_var_a, &rule_var_b}}};
    expr g_body{expr::functor{"g", {&rule_var_a}}};
    expr h_body{expr::functor{"h", {&rule_var_b}}};
    expr g_head{expr::functor{"g", {&abc}}};
    expr h_head{expr::functor{"h", {&_123}}};
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {}});
    database.push(rule{&h_head, {}});

    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const uint32_t idx_a = seq.next();
    const uint32_t idx_b = seq.next();
    const expr* var_a = make_var.make(idx_a);
    const expr* var_b = make_var.make(idx_b);
    initial_goals.push(make_functor.make("f", {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf(var_b)->content);
    EXPECT_EQ(whnf_a.name, "abc");
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.name, "123");
    EXPECT_TRUE(whnf_b.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterDecisionBindingVarsFromChosenFact) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(abc, 123).
     *   1: f(xyz, 456).
     * setup: decision picks rule 1
     */
    expr abc{expr::functor{"abc", {}}};
    expr xyz{expr::functor{"xyz", {}}};
    expr _123{expr::functor{"123", {}}};
    expr _456{expr::functor{"456", {}}};
    expr head0{expr::functor{"f", {&abc, &_123}}};
    expr head1{expr::functor{"f", {&xyz, &_456}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    const goal_lineage* gl = stack.loc.locate<i_make_initial_goal_lineage>().make(0);
    const resolution_lineage* chosen =
        stack.loc.locate<i_make_resolution_lineage>().make_resolution_lineage(gl, rule_id{1});

    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(chosen));

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const uint32_t idx_a = seq.next();
    const uint32_t idx_b = seq.next();
    const expr* var_a = make_var.make(idx_a);
    const expr* var_b = make_var.make(idx_b);
    initial_goals.push(make_functor.make("f", {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf(var_b)->content);
    EXPECT_EQ(whnf_a.name, "xyz");
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.name, "456");
    EXPECT_TRUE(whnf_b.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenMhuRejectsInconsistentRuleWithoutDecision) {
    /*
     * initial goals:
     *   f(A, A).
     * database:
     *   0: f(abc, abc).
     *   1: f(abc, 123).
     */
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head0{expr::functor{"f", {&abc, &abc}}};
    expr head1{expr::functor{"f", {&abc, &_123}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();
    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();
    i_derive_resolution_lemma& derive_resolution_lemma =
        stack.loc.locate<i_derive_resolution_lemma>();

    const goal_lineage* gl = make_initial_goal_lineage.make(0);
    const resolution_lineage* rl0 =
        make_resolution_lineage.make_resolution_lineage(gl, rule_id{0});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const uint32_t idx_a = seq.next();
    const expr* var_a = make_var.make(idx_a);
    initial_goals.push(make_functor.make("f", {var_a, var_a}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
    EXPECT_EQ(whnf_a.name, "abc");
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl0));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunDeactivatesRuleOneWhenDecisionResolvesRuleZeroOnMhuSharedGoal) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(abc, 123).
     *   1: f(def, 123).
     * setup: decision picks rule 0; MHU constrain on rule 0 should purge rule 1 head
     */
    expr abc{expr::functor{"abc", {}}};
    expr def{expr::functor{"def", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head0{expr::functor{"f", {&abc, &_123}}};
    expr head1{expr::functor{"f", {&def, &_123}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {}});

    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();
    i_derive_resolution_lemma& derive_resolution_lemma =
        stack.loc.locate<i_derive_resolution_lemma>();
    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    const goal_lineage* gl = make_initial_goal_lineage.make(0);
    const resolution_lineage* rl0 =
        make_resolution_lineage.make_resolution_lineage(gl, rule_id{0});
    const resolution_lineage* rl1 =
        make_resolution_lineage.make_resolution_lineage(gl, rule_id{1});

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl0));

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = make_var.make(seq.next());
    const expr* var_b = make_var.make(seq.next());
    initial_goals.push(make_functor.make("f", {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl0));
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf(var_b)->content);
    EXPECT_EQ(whnf_a.name, "abc");
    EXPECT_EQ(whnf_b.name, "123");
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunDeactivatesCrossGoalCandidateOnMhuIncompatibleHead) {
    /*
     * initial goals (two subgoals, shared var A):
     *   f(A).
     *   g(A).
     * database:
     *   0: f(abc).
     *   1: f(xyz).
     *   2: g(def).
     *   3: g(abc).
     * setup: both goals non-unit; decision picks goal 0 rule 0; MHU eliminates goal 1 rule 2
     */
    expr abc{expr::functor{"abc", {}}};
    expr xyz{expr::functor{"xyz", {}}};
    expr def{expr::functor{"def", {}}};
    expr head_f0{expr::functor{"f", {&abc}}};
    expr head_f1{expr::functor{"f", {&xyz}}};
    expr head_g2{expr::functor{"g", {&def}}};
    expr head_g3{expr::functor{"g", {&abc}}};
    database.push(rule{&head_f0, {}});
    database.push(rule{&head_f1, {}});
    database.push(rule{&head_g2, {}});
    database.push(rule{&head_g3, {}});

    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();
    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    const goal_lineage* gl0 = make_initial_goal_lineage.make(0);
    const resolution_lineage* rl_f0 =
        make_resolution_lineage.make_resolution_lineage(gl0, rule_id{0});

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_f0));

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = make_var.make(seq.next());
    initial_goals.push(make_functor.make("f", {var_a}));
    initial_goals.push(make_functor.make("g", {var_a}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
    EXPECT_EQ(whnf_a.name, "abc");
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedBindingVarInNestedFunctorArgWithoutDecisions) {
    /*
     * initial goals:
     *   f(g(A)).
     * database:
     *   0: f(g(abc)).
     */
    expr abc{expr::functor{"abc", {}}};
    expr g_abc{expr::functor{"g", {&abc}}};
    expr head{expr::functor{"f", {&g_abc}}};
    database.push(rule{&head, {}});

    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const uint32_t idx_a = seq.next();
    const expr* var_a = make_var.make(idx_a);
    const expr* g_a = make_functor.make("g", {var_a});
    initial_goals.push(make_functor.make("f", {g_a}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
    EXPECT_EQ(whnf_a.name, "abc");
    EXPECT_TRUE(whnf_a.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedBindingRemainingVarWhenGoalIsPartiallyGround) {
    /*
     * initial goals:
     *   f(abc, B).
     * database:
     *   0: f(abc, 123).
     */
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr head{expr::functor{"f", {&abc, &_123}}};
    database.push(rule{&head, {}});

    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* abc_pool = make_functor.make("abc", {});
    const uint32_t idx_b = seq.next();
    const expr* var_b = make_var.make(idx_b);
    initial_goals.push(make_functor.make("f", {abc_pool, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf(var_b)->content);
    EXPECT_EQ(whnf_b.name, "123");
    EXPECT_TRUE(whnf_b.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenClauseBodyGoalHasNoUnifyingFact) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(C, D) :- g(C), h(D).
     *   1: g(abc).
     */
    expr rule_var_a{expr::var{0}};
    expr rule_var_b{expr::var{1}};
    expr abc{expr::functor{"abc", {}}};
    expr f_head{expr::functor{"f", {&rule_var_a, &rule_var_b}}};
    expr g_body{expr::functor{"g", {&rule_var_a}}};
    expr h_body{expr::functor{"h", {&rule_var_b}}};
    expr g_head{expr::functor{"g", {&abc}}};
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = make_var.make(seq.next());
    const expr* var_b = make_var.make(seq.next());
    initial_goals.push(make_functor.make("f", {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenClauseBodyFactMismatchesGroundGoal) {
    /*
     * initial goals:
     *   f(abc, 123).
     * database:
     *   0: f(A, B) :- g(A), h(B).
     *   1: g(abc).
     *   2: h(234).
     */
    expr rule_var_a{expr::var{0}};
    expr rule_var_b{expr::var{1}};
    expr abc{expr::functor{"abc", {}}};
    expr two_three_four{expr::functor{"234", {}}};
    expr f_head{expr::functor{"f", {&rule_var_a, &rule_var_b}}};
    expr g_body{expr::functor{"g", {&rule_var_a}}};
    expr h_body{expr::functor{"h", {&rule_var_b}}};
    expr g_head{expr::functor{"g", {&abc}}};
    expr h_head{expr::functor{"h", {&two_three_four}}};
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head, {}});
    database.push(rule{&h_head, {}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    initial_goals.push(make_functor.make("f",
        {make_functor.make("abc", {}), make_functor.make("123", {})}));

    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterDecisionOnBodyGoalWithTwoFacts) {
    /*
     * initial goals:
     *   f(A, B).
     * database:
     *   0: f(C, D) :- g(C), h(D).
     *   1: g(abc).
     *   2: g(xyz).
     *   3: h(123).
     * setup: decision picks rule 2 for subgoal g
     */
    expr rule_var_a{expr::var{0}};
    expr rule_var_b{expr::var{1}};
    expr abc{expr::functor{"abc", {}}};
    expr xyz{expr::functor{"xyz", {}}};
    expr _123{expr::functor{"123", {}}};
    expr f_head{expr::functor{"f", {&rule_var_a, &rule_var_b}}};
    expr g_body{expr::functor{"g", {&rule_var_a}}};
    expr h_body{expr::functor{"h", {&rule_var_b}}};
    expr g_head0{expr::functor{"g", {&abc}}};
    expr g_head1{expr::functor{"g", {&xyz}}};
    expr h_head{expr::functor{"h", {&_123}}};
    database.push(rule{&f_head, {&g_body, &h_body}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});
    database.push(rule{&h_head, {}});

    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();
    i_make_goal_lineage& make_goal_lineage = stack.loc.locate<i_make_goal_lineage>();
    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
    const resolution_lineage* rl_f =
        make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
    const goal_lineage* gl_g = make_goal_lineage.make_goal_lineage(rl_f, subgoal_id{0});
    const resolution_lineage* chosen_g =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{2});

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(chosen_g));

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = make_var.make(seq.next());
    const expr* var_b = make_var.make(seq.next());
    initial_goals.push(make_functor.make("f", {var_a, var_b}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf(var_b)->content);
    EXPECT_EQ(whnf_a.name, "xyz");
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.name, "123");
    EXPECT_TRUE(whnf_b.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenSharedVarLinksTwoGoalsWithoutDecisions) {
    /*
     * initial goals:
     *   f(A, B).
     *   g(B, C).
     * database:
     *   0: f(abc, 123).
     *   1: g(456, 789).
     *   2: g(123, xyz).
     */
    expr abc{expr::functor{"abc", {}}};
    expr _123{expr::functor{"123", {}}};
    expr _456{expr::functor{"456", {}}};
    expr _789{expr::functor{"789", {}}};
    expr xyz{expr::functor{"xyz", {}}};
    expr f_head{expr::functor{"f", {&abc, &_123}}};
    expr g_head0{expr::functor{"g", {&_456, &_789}}};
    expr g_head1{expr::functor{"g", {&_123, &xyz}}};
    database.push(rule{&f_head, {}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});

    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = make_var.make(seq.next());
    const expr* var_b = make_var.make(seq.next());
    const expr* var_c = make_var.make(seq.next());
    initial_goals.push(make_functor.make("f", {var_a, var_b}));
    initial_goals.push(make_functor.make("g", {var_b, var_c}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
    const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf(var_b)->content);
    const expr::functor& whnf_c = std::get<expr::functor>(bind_map.whnf(var_c)->content);
    EXPECT_EQ(whnf_a.name, "abc");
    EXPECT_TRUE(whnf_a.args.empty());
    EXPECT_EQ(whnf_b.name, "123");
    EXPECT_TRUE(whnf_b.args.empty());
    EXPECT_EQ(whnf_c.name, "xyz");
    EXPECT_TRUE(whnf_c.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenCdclAndMhuReduceGoalGCandidatesWithoutDecisions) {
    /*
     * initial goals:
     *   f.
     *   g(A, xyz).
     * database:
     *   0: f.
     *   1: g(abc, xyz).
     *   2: g(def, xyz).
     *   3: g(ghi, jkl).
     * setup: CDCL avoidance {{goal f, rule 0}, {goal g, rule 1}}
     */
    expr abc{expr::functor{"abc", {}}};
    expr def{expr::functor{"def", {}}};
    expr ghi{expr::functor{"ghi", {}}};
    expr _xyz{expr::functor{"xyz", {}}};
    expr _jkl{expr::functor{"jkl", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr g_head1{expr::functor{"g", {&abc, &_xyz}}};
    expr g_head2{expr::functor{"g", {&def, &_xyz}}};
    expr g_head3{expr::functor{"g", {&ghi, &_jkl}}};
    database.push(rule{&f_head, {}});
    database.push(rule{&g_head1, {}});
    database.push(rule{&g_head2, {}});
    database.push(rule{&g_head3, {}});

    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();
    i_learn_avoidance& learn_avoidance = stack.loc.locate<i_learn_avoidance>();
    i_derive_resolution_lemma& derive_resolution_lemma =
        stack.loc.locate<i_derive_resolution_lemma>();
    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
    const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
    const resolution_lineage* rl_f_0 =
        make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
    const resolution_lineage* rl_g_1 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{1});
    const resolution_lineage* rl_g_2 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{2});

    learn_avoidance.learn(lemma{{rl_f_0, rl_g_1}});

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = make_var.make(seq.next());
    const expr* xyz = make_functor.make("xyz", {});
    initial_goals.push(make_functor.make("f", {}));
    initial_goals.push(make_functor.make("g", {var_a, xyz}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_f_0, rl_g_2));
    const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
    EXPECT_EQ(whnf_a.name, "def");
    EXPECT_TRUE(whnf_a.args.empty());
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedBuildingListOfFiveAbcWithoutDecisions) {
    /*
     * initial goals:
     *   make_list(suc(suc(suc(suc(suc(zero))))), abc, R).
     * database:
     *   0: make_list(zero, _, nil).
     *   1: make_list(suc(L), A, cons(A, T)) :- make_list(L, A, T).
     */
    expr rule_ignored{expr::var{0}};
    expr rule_l{expr::var{0}};
    expr rule_a{expr::var{1}};
    expr rule_t{expr::var{2}};
    expr zero{expr::functor{"zero", {}}};
    expr nil{expr::functor{"nil", {}}};
    expr head0{expr::functor{"make_list", {&zero, &rule_ignored, &nil}}};
    expr suc_l{expr::functor{"suc", {&rule_l}}};
    expr cons_at{expr::functor{"cons", {&rule_a, &rule_t}}};
    expr head1{expr::functor{"make_list", {&suc_l, &rule_a, &cons_at}}};
    expr body1{expr::functor{"make_list", {&rule_l, &rule_a, &rule_t}}};
    database.push(rule{&head0, {}});
    database.push(rule{&head1, {&body1}});

    i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    static constexpr size_t kMaxResolutions = 32;
    static constexpr int kListLength = 5;
    simulation simulation{stack.loc, kMaxResolutions};
    simulation.set_up();
    const expr* zero_pool = make_functor.make("zero", {});
    const expr* len = zero_pool;
    for (int i = 0; i < kListLength; ++i)
        len = make_functor.make("suc", {len});
    const expr* abc = make_functor.make("abc", {});
    const expr* var_r = make_var.make(seq.next());
    initial_goals.push(make_functor.make("make_list", {len, abc, var_r}));

    EXPECT_EQ(simulation.run(), sim_termination::solved);

    const expr* tail = bind_map.whnf(var_r);
    for (int i = 0; i < kListLength; ++i) {
        const expr::functor& cell = std::get<expr::functor>(tail->content);
        ASSERT_EQ(cell.name, "cons");
        ASSERT_EQ(cell.args.size(), 2u);
        const expr::functor& head =
            std::get<expr::functor>(bind_map.whnf(cell.args[0])->content);
        EXPECT_EQ(head.name, "abc");
        EXPECT_TRUE(head.args.empty());
        tail = bind_map.whnf(cell.args[1]);
    }
    const expr::functor& nil_tail = std::get<expr::functor>(bind_map.whnf(tail)->content);
    EXPECT_EQ(nil_tail.name, "nil");
    EXPECT_TRUE(nil_tail.args.empty());

    simulation.tear_down();
}

// ---------------------------------------------------------------------------
// round 2 gaps — depth, multi-decision, CDCL mid-loop conflict, partial progress
// ---------------------------------------------------------------------------

TEST_F(SimIntegrationTest, RunReturnsConflictedWhenCdclEliminationExhaustsSiblingGoal) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     *   2: g.
     *   3: g.
     * setup: two avoidances sharing f/0 eliminate both g candidates on constrain(f/0);
     *        conflict mirror of RunReturnsSolvedAfterCdclAvoidanceForcesG1RuleThree
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head0{expr::functor{"g", {}}};
    expr g_head1{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});

    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();
    i_learn_avoidance& learn_avoidance = stack.loc.locate<i_learn_avoidance>();
    i_derive_resolution_lemma& derive_resolution_lemma =
        stack.loc.locate<i_derive_resolution_lemma>();
    i_get_resolution_count& get_resolution_count =
        stack.loc.locate<i_get_resolution_count>();

    const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
    const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
    const resolution_lineage* rl_f_0 =
        make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
    const resolution_lineage* rl_g_0 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{2});
    const resolution_lineage* rl_g_1 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{3});

    learn_avoidance.learn(lemma{{rl_f_0, rl_g_0}});
    learn_avoidance.learn(lemma{{rl_f_0, rl_g_1}});

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_f_0));

    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    EXPECT_EQ(get_resolution_count.get_resolution_count(), 1u);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_f_0));
    simulation.tear_down();
}

// Integration counterpart of unit RunReturnsDepthExceededWhenNoSolutionWithinLimit.
TEST_F(SimIntegrationTest, RunReturnsDepthExceededOnSelfRecursiveClause) {
    /*
     * initial goals:
     *   f.
     * database:
     *   0: f :- f.
     */
    expr goal{expr::functor{"f", {}}};
    expr f_head{expr::functor{"f", {}}};
    expr f_body{expr::functor{"f", {}}};
    initial_goals.push(&goal);
    database.push(rule{&f_head, {&f_body}});

    static constexpr size_t kMaxResolutions = 4;
    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
    EXPECT_EQ(stack.loc.locate<i_get_resolution_count>().get_resolution_count(), kMaxResolutions);
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedAfterTwoSequentialDecisions) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     *   2: g.
     *   3: g.
     * setup: second decision on g after f branch committed
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head0{expr::functor{"g", {}}};
    expr g_head1{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});

    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();
    i_derive_resolution_lemma& derive_resolution_lemma =
        stack.loc.locate<i_derive_resolution_lemma>();
    i_get_decision_count& get_decision_count =
        stack.loc.locate<i_get_decision_count>();

    const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
    const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
    const resolution_lineage* rl_f_0 =
        make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
    const resolution_lineage* rl_g_1 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{3});

    // Script decision choices only; behavioral contract is termination + recorded decisions/resolutions.
    EXPECT_CALL(stack.decision_generator, generate())
        .WillOnce(Return(rl_f_0))
        .WillOnce(Return(rl_g_1));

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_EQ(get_decision_count.count(), 2u);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_f_0, rl_g_1));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedWhenCdclUnitElimForcesRemainingCandidate) {
    /*
     * initial goals:
     *   f.
     *   g.
     * database:
     *   0: f.
     *   1: f.
     *   2: g.
     *   3: g.
     * setup: CDCL unit elim g/2 on constrain(f/0) leaves g unit on g/3; no second decision
     */
    expr goal_f{expr::functor{"f", {}}};
    expr goal_g{expr::functor{"g", {}}};
    expr f_head0{expr::functor{"f", {}}};
    expr f_head1{expr::functor{"f", {}}};
    expr g_head0{expr::functor{"g", {}}};
    expr g_head1{expr::functor{"g", {}}};
    initial_goals.push(&goal_f);
    initial_goals.push(&goal_g);
    database.push(rule{&f_head0, {}});
    database.push(rule{&f_head1, {}});
    database.push(rule{&g_head0, {}});
    database.push(rule{&g_head1, {}});

    i_make_initial_goal_lineage& make_initial_goal_lineage =
        stack.loc.locate<i_make_initial_goal_lineage>();
    i_make_resolution_lineage& make_resolution_lineage =
        stack.loc.locate<i_make_resolution_lineage>();
    i_learn_avoidance& learn_avoidance = stack.loc.locate<i_learn_avoidance>();
    i_derive_resolution_lemma& derive_resolution_lemma =
        stack.loc.locate<i_derive_resolution_lemma>();

    const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
    const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
    const resolution_lineage* rl_f_0 =
        make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
    const resolution_lineage* rl_g_2 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{2});
    const resolution_lineage* rl_g_3 =
        make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{3});

    learn_avoidance.learn(lemma{{rl_f_0, rl_g_2}});

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();

    EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_f_0));

    EXPECT_EQ(simulation.run(), sim_termination::solved);
    EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
        UnorderedElementsAre(rl_f_0, rl_g_3));
    simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsConflictedAfterPartialProgressWhenDerivedGoalFails) {
    /*
     * initial goals:
     *   f(A).
     *   g(A).
     * database:
     *   0: f(A) :- h(A).
     *   1: h(abc).
     *   2: g(xyz).
     * setup: shared A; f/h and g(xyz) cannot both succeed — conflict after unit resolution(s)
     */
    expr rule_var_a{expr::var{0}};
    expr abc{expr::functor{"abc", {}}};
    expr xyz{expr::functor{"xyz", {}}};
    expr f_head{expr::functor{"f", {&rule_var_a}}};
    expr h_body{expr::functor{"h", {&rule_var_a}}};
    expr h_head{expr::functor{"h", {&abc}}};
    expr g_head{expr::functor{"g", {&xyz}}};
    database.push(rule{&f_head, {&h_body}});
    database.push(rule{&h_head, {}});
    database.push(rule{&g_head, {}});

    i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
    i_make_var& make_var = stack.loc.locate<i_make_var>();
    i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

    EXPECT_CALL(stack.decision_generator, generate()).Times(0);

    simulation simulation{stack.loc, kDefaultMaxResolutions};
    simulation.set_up();
    const expr* var_a = make_var.make(seq.next());
    initial_goals.push(make_functor.make("f", {var_a}));
    initial_goals.push(make_functor.make("g", {var_a}));

    EXPECT_EQ(simulation.run(), sim_termination::conflicted);
    simulation.tear_down();
}

// ---------------------------------------------------------------------------
// sim lifecycle (trail + store observables)
// ---------------------------------------------------------------------------

TEST_F(SimIntegrationTest, SetUpLifecyclePushesOneTrailFrameWithoutRunning) {
  // Capture baseline immediately before set_up(); wiring must not intern or push frames.
  const size_t depth_before = stack.early.trail_.depth();
  const size_t expr_before = stack.loc.locate<i_get_expr_count>().size();

  simulation simulation{stack.loc, kDefaultMaxResolutions};
  simulation.set_up();

  EXPECT_EQ(stack.early.trail_.depth(), depth_before + 1);
  EXPECT_EQ(stack.loc.locate<i_get_expr_count>().size(), expr_before);
  EXPECT_TRUE(stack.loc.locate<i_check_active_goals_empty>().empty());
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, TearDownLifecycleRestoresTrailDepthAfterEmptyRun) {
  const size_t depth_before = stack.early.trail_.depth();

  simulation simulation{stack.loc, kDefaultMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  simulation.tear_down();

  EXPECT_EQ(stack.early.trail_.depth(), depth_before);
}

TEST_F(SimIntegrationTest, TearDownLifecycleRestoresTrailDepthAfterConflictedRun) {
  expr goal{expr::functor{"f", {}}};
  initial_goals.push(&goal);

  const size_t depth_before = stack.early.trail_.depth();

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kDefaultMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::conflicted);
  simulation.tear_down();

  EXPECT_EQ(stack.early.trail_.depth(), depth_before);
}

TEST_F(SimIntegrationTest, TearDownLifecycleRestoresTrailDepthAfterDepthExceededRun) {
  expr goal{expr::functor{"f", {}}};
  expr f_head{expr::functor{"f", {}}};
  expr f_body{expr::functor{"f", {}}};
  initial_goals.push(&goal);
  database.push(rule{&f_head, {&f_body}});

  static constexpr size_t kMaxResolutions = 4;
  const size_t depth_before = stack.early.trail_.depth();

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
  simulation.tear_down();

  EXPECT_EQ(stack.early.trail_.depth(), depth_before);
}

TEST_F(SimIntegrationTest, TearDownLifecycleClearsEphemeralStoresAfterSolvedRun) {
  expr abc{expr::functor{"abc", {}}};
  expr _123{expr::functor{"123", {}}};
  expr head{expr::functor{"f", {&abc, &_123}}};
  database.push(rule{&head, {}});

  i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
  i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
  i_make_var& make_var = stack.loc.locate<i_make_var>();
  i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

  const uint32_t idx_test = seq.next();
  const expr* test_var = make_var.make(idx_test);
  initial_goals.push(make_functor.make("f", {test_var, make_var.make(seq.next())}));

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kDefaultMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  simulation.tear_down();

  EXPECT_TRUE(stack.loc.locate<i_check_active_goals_empty>().empty());
  EXPECT_EQ(stack.loc.locate<i_get_decision_count>().count(), 0u);
  EXPECT_EQ(stack.loc.locate<i_get_resolution_count>().get_resolution_count(), 0u);
  EXPECT_THAT(stack.loc.locate<i_derive_resolution_lemma>().derive_resolution_lemma().get_resolutions(),
      IsEmpty());
  const expr* whnf = bind_map.whnf(test_var);
  ASSERT_TRUE(std::holds_alternative<expr::var>(whnf->content));
  EXPECT_EQ(std::get<expr::var>(whnf->content).index, idx_test);
}

TEST_F(SimIntegrationTest, TearDownLifecyclePopUndoesInFrameExprPoolGrowth) {
  expr rule_ignored{expr::var{0}};
  expr rule_l{expr::var{0}};
  expr rule_a{expr::var{1}};
  expr rule_t{expr::var{2}};
  expr zero{expr::functor{"zero", {}}};
  expr nil{expr::functor{"nil", {}}};
  expr head0{expr::functor{"make_list", {&zero, &rule_ignored, &nil}}};
  expr suc_l{expr::functor{"suc", {&rule_l}}};
  expr cons_at{expr::functor{"cons", {&rule_a, &rule_t}}};
  expr head1{expr::functor{"make_list", {&suc_l, &rule_a, &cons_at}}};
  expr body1{expr::functor{"make_list", {&rule_l, &rule_a, &rule_t}}};
  database.push(rule{&head0, {}});
  database.push(rule{&head1, {&body1}});

  i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
  i_make_var& make_var = stack.loc.locate<i_make_var>();
  i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

  const size_t expr_before = stack.loc.locate<i_get_expr_count>().size();

  static constexpr size_t kMaxResolutions = 32;
  static constexpr int kListLength = 5;
  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kMaxResolutions};
  simulation.set_up();
  const expr* zero_pool = make_functor.make("zero", {});
  const expr* len = zero_pool;
  for (int i = 0; i < kListLength; ++i)
    len = make_functor.make("suc", {len});
  const expr* abc = make_functor.make("abc", {});
  const expr* var_r = make_var.make(seq.next());
  initial_goals.push(make_functor.make("make_list", {len, abc, var_r}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_GT(stack.loc.locate<i_get_expr_count>().size(), expr_before);

  simulation.tear_down();
  EXPECT_EQ(stack.loc.locate<i_get_expr_count>().size(), expr_before);
}

TEST_F(SimIntegrationTest, TearDownLifecycleResetsVarSequencerWhenIncrementedInFrame) {
  expr rule_var{expr::var{0}};
  expr abc{expr::functor{"abc", {}}};
  expr f_head{expr::functor{"f", {&rule_var}}};
  expr g_body{expr::functor{"g", {&rule_var}}};
  expr g_head{expr::functor{"g", {&abc}}};
  database.push(rule{&f_head, {&g_body}});
  database.push(rule{&g_head, {}});

  i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
  i_make_var& make_var = stack.loc.locate<i_make_var>();
  i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

  (void)seq.next();
  (void)seq.next();
  static constexpr uint32_t kExpectedAfterTeardown = 2;

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kDefaultMaxResolutions};
  simulation.set_up();
  const expr* var_in_frame = make_var.make(seq.next());
  initial_goals.push(make_functor.make("f", {var_in_frame}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  simulation.tear_down();

  EXPECT_EQ(seq.next(), kExpectedAfterTeardown);
}

TEST_F(SimIntegrationTest, BaseFrameCdclLearnSurvivesLifecycleTearDown) {
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
  i_pin_resolution_lineage& pin_resolution_lineage =
      stack.loc.locate<i_pin_resolution_lineage>();
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
  pin_resolution_lineage.pin(rl_g0_0);
  pin_resolution_lineage.pin(rl_g1_2);
  pin_resolution_lineage.pin(rl_g1_3);

  simulation simulation{stack.loc, kDefaultMaxResolutions};
  simulation.set_up();

  EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_g0_0));
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
      UnorderedElementsAre(rl_g0_0, rl_g1_3));
  simulation.tear_down();

  simulation.set_up();
  EXPECT_CALL(stack.decision_generator, generate()).WillOnce(Return(rl_g0_0));
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_THAT(derive_resolution_lemma.derive_resolution_lemma().get_resolutions(),
      UnorderedElementsAre(rl_g0_0, rl_g1_3));
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, IdenticalSimCycleLifecycleRunsCleanAfterTearDown) {
  expr abc{expr::functor{"abc", {}}};
  expr _123{expr::functor{"123", {}}};
  expr head{expr::functor{"f", {&abc, &_123}}};
  database.push(rule{&head, {}});

  i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
  i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
  i_make_var& make_var = stack.loc.locate<i_make_var>();
  i_make_functor& make_functor = stack.loc.locate<i_make_functor>();
  i_get_resolution_count& get_resolution_count =
      stack.loc.locate<i_get_resolution_count>();
  i_get_decision_count& get_decision_count = stack.loc.locate<i_get_decision_count>();

  const uint32_t idx_a = seq.next();
  const uint32_t idx_b = seq.next();
  const expr* var_a = make_var.make(idx_a);
  const expr* var_b = make_var.make(idx_b);
  const expr* goal = make_functor.make("f", {var_a, var_b});
  initial_goals.push(goal);

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kDefaultMaxResolutions};

  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  const size_t res_count_1 = get_resolution_count.get_resolution_count();
  const size_t dec_count_1 = get_decision_count.count();
  const std::string whnf_a_1 =
      std::get<expr::functor>(bind_map.whnf(var_a)->content).name;
  const std::string whnf_b_1 =
      std::get<expr::functor>(bind_map.whnf(var_b)->content).name;
  simulation.tear_down();

  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_EQ(get_resolution_count.get_resolution_count(), res_count_1);
  EXPECT_EQ(get_decision_count.count(), dec_count_1);
  const std::string whnf_a_2 =
      std::get<expr::functor>(bind_map.whnf(var_a)->content).name;
  const std::string whnf_b_2 =
      std::get<expr::functor>(bind_map.whnf(var_b)->content).name;
  EXPECT_EQ(whnf_a_2, whnf_a_1);
  EXPECT_EQ(whnf_b_2, whnf_b_1);
  simulation.tear_down();
}

// ---------------------------------------------------------------------------
// sim stress
// ---------------------------------------------------------------------------

TEST_F(SimIntegrationTest, RunReturnsSolvedStressListOfTwentyAbcWithoutDecisions) {
  expr rule_ignored{expr::var{0}};
  expr rule_l{expr::var{0}};
  expr rule_a{expr::var{1}};
  expr rule_t{expr::var{2}};
  expr zero{expr::functor{"zero", {}}};
  expr nil{expr::functor{"nil", {}}};
  expr head0{expr::functor{"make_list", {&zero, &rule_ignored, &nil}}};
  expr suc_l{expr::functor{"suc", {&rule_l}}};
  expr cons_at{expr::functor{"cons", {&rule_a, &rule_t}}};
  expr head1{expr::functor{"make_list", {&suc_l, &rule_a, &cons_at}}};
  expr body1{expr::functor{"make_list", {&rule_l, &rule_a, &rule_t}}};
  database.push(rule{&head0, {}});
  database.push(rule{&head1, {&body1}});

  i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
  i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
  i_make_var& make_var = stack.loc.locate<i_make_var>();
  i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

  static constexpr size_t kMaxResolutions = 64;
  static constexpr int kListLength = 20;
  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kMaxResolutions};
  simulation.set_up();
  const expr* zero_pool = make_functor.make("zero", {});
  const expr* len = make_suc_n(make_functor, zero_pool, kListLength);
  const expr* abc = make_functor.make("abc", {});
  const expr* var_r = make_var.make(seq.next());
  initial_goals.push(make_functor.make("make_list", {len, abc, var_r}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);

  const expr* tail = bind_map.whnf(var_r);
  for (int i = 0; i < kListLength; ++i) {
    const expr::functor& cell = std::get<expr::functor>(tail->content);
    ASSERT_EQ(cell.name, "cons");
    ASSERT_EQ(cell.args.size(), 2u);
    const expr::functor& head_cell =
        std::get<expr::functor>(bind_map.whnf(cell.args[0])->content);
    EXPECT_EQ(head_cell.name, "abc");
    EXPECT_TRUE(head_cell.args.empty());
    tail = bind_map.whnf(cell.args[1]);
  }
  const expr::functor& nil_tail = std::get<expr::functor>(bind_map.whnf(tail)->content);
  EXPECT_EQ(nil_tail.name, "nil");
  EXPECT_TRUE(nil_tail.args.empty());
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressLinearChainClauseDepthTwenty) {
  static constexpr size_t kChainDepth = 20;
  static constexpr size_t kMaxResolutions = 64;

  expr ground{expr::functor{"abc", {}}};
  std::vector<expr> storage;
  chain_clause_db(database, storage, "chain", kChainDepth, &ground);

  i_get_resolution_count& get_resolution_count =
      stack.loc.locate<i_get_resolution_count>();

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  expr goal{expr::functor{"chain0", {&ground}}};
  initial_goals.push(&goal);

  simulation simulation{stack.loc, kMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_GE(get_resolution_count.get_resolution_count(), kChainDepth);
  EXPECT_LE(get_resolution_count.get_resolution_count(), kMaxResolutions);
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressEvenOddPeanoGoal) {
  expr rule_x{expr::var{0}};
  expr zero{expr::functor{"zero", {}}};
  expr suc_x{expr::functor{"suc", {&rule_x}}};
  expr even_head0{expr::functor{"even", {&zero}}};
  expr odd_head{expr::functor{"odd", {&suc_x}}};
  expr even_head1{expr::functor{"even", {&suc_x}}};
  expr odd_body{expr::functor{"odd", {&rule_x}}};
  expr even_body{expr::functor{"even", {&rule_x}}};
  database.push(rule{&even_head0, {}});
  database.push(rule{&odd_head, {&even_body}});
  database.push(rule{&even_head1, {&odd_body}});

  i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

  static constexpr size_t kMaxResolutions = 128;
  static constexpr int kSucDepth = 12;
  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kMaxResolutions};
  simulation.set_up();
  const expr* zero_pool = make_functor.make("zero", {});
  const expr* goal_even = make_functor.make("even", {make_suc_n(make_functor, zero_pool, kSucDepth)});
  initial_goals.push(goal_even);

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressDeepNestedFunctorTower) {
  static constexpr int kTowerDepth = 8;
  static constexpr size_t kMaxResolutions = 64;
  expr zero{expr::functor{"zero", {}}};
  expr rule_x{expr::var{0}};
  expr suc_wrap{expr::functor{"wrap", {&rule_x}}};
  expr unwrap_head{expr::functor{"unwrap", {&suc_wrap}}};
  expr unwrap_body{expr::functor{"unwrap", {&rule_x}}};
  expr unwrap_zero{expr::functor{"unwrap", {&zero}}};
  database.push(rule{&unwrap_head, {&unwrap_body}});
  database.push(rule{&unwrap_zero, {}});

  i_make_functor& make_functor = stack.loc.locate<i_make_functor>();
  i_get_resolution_count& get_resolution_count =
      stack.loc.locate<i_get_resolution_count>();

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kMaxResolutions};
  simulation.set_up();
  const expr* inner = &zero;
  for (int i = 0; i < kTowerDepth; ++i)
    inner = make_functor.make("wrap", {inner});
  const expr* goal_expr = make_functor.make("unwrap", {inner});
  initial_goals.push(goal_expr);

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_GE(get_resolution_count.get_resolution_count(), static_cast<size_t>(kTowerDepth));
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressLongSharedVarChainWithoutDecisions) {
  static constexpr int kChainGoals = 6;
  expr t0{expr::functor{"t0", {}}};
  expr t1{expr::functor{"t1", {}}};
  expr t2{expr::functor{"t2", {}}};
  expr t3{expr::functor{"t3", {}}};
  expr t4{expr::functor{"t4", {}}};
  expr t5{expr::functor{"t5", {}}};
  expr t6{expr::functor{"t6", {}}};
  expr g0_head{expr::functor{"g0", {&t0, &t1}}};
  expr g1_head{expr::functor{"g1", {&t1, &t2}}};
  expr g2_head{expr::functor{"g2", {&t2, &t3}}};
  expr g3_head{expr::functor{"g3", {&t3, &t4}}};
  expr g4_head{expr::functor{"g4", {&t4, &t5}}};
  expr g5_head{expr::functor{"g5", {&t5, &t6}}};
  database.push(rule{&g0_head, {}});
  database.push(rule{&g1_head, {}});
  database.push(rule{&g2_head, {}});
  database.push(rule{&g3_head, {}});
  database.push(rule{&g4_head, {}});
  database.push(rule{&g5_head, {}});

  i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
  i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
  i_make_var& make_var = stack.loc.locate<i_make_var>();
  i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kDefaultMaxResolutions};
  simulation.set_up();
  std::vector<const expr*> vars;
  for (int i = 0; i <= kChainGoals; ++i)
    vars.push_back(make_var.make(seq.next()));
  for (int i = 0; i < kChainGoals; ++i)
    initial_goals.push(make_functor.make(
        ("g" + std::to_string(i)).c_str(), {vars[i], vars[i + 1]}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  const expr::functor& whnf_end =
      std::get<expr::functor>(bind_map.whnf(vars[kChainGoals])->content);
  EXPECT_EQ(whnf_end.name, "t6");
  EXPECT_TRUE(whnf_end.args.empty());
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressDiamondSharedVarWithoutDecisions) {
  expr abc{expr::functor{"abc", {}}};
  expr _123{expr::functor{"123", {}}};
  expr xyz{expr::functor{"xyz", {}}};
  expr f_head{expr::functor{"f", {&abc, &xyz}}};
  expr g_head{expr::functor{"g", {&abc, &_123}}};
  expr h_head{expr::functor{"h", {&_123, &xyz}}};
  database.push(rule{&f_head, {}});
  database.push(rule{&g_head, {}});
  database.push(rule{&h_head, {}});

  i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
  i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
  i_make_var& make_var = stack.loc.locate<i_make_var>();
  i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kDefaultMaxResolutions};
  simulation.set_up();
  const expr* var_a = make_var.make(seq.next());
  const expr* var_b = make_var.make(seq.next());
  const expr* var_c = make_var.make(seq.next());
  initial_goals.push(make_functor.make("f", {var_a, var_c}));
  initial_goals.push(make_functor.make("g", {var_a, var_b}));
  initial_goals.push(make_functor.make("h", {var_b, var_c}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
  const expr::functor& whnf_c = std::get<expr::functor>(bind_map.whnf(var_c)->content);
  EXPECT_EQ(whnf_a.name, "abc");
  EXPECT_EQ(whnf_c.name, "xyz");
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressWideClauseTreeWithoutDecisions) {
  // Base tree: f:-g,h; g:-i,j; h:-i,j; facts i,j. Wide: extra clause k:-i,j,l; l. fact.
  static constexpr size_t kMinResolutions = 7;

  expr goal_f{expr::functor{"f", {}}};
  expr f_head{expr::functor{"f", {}}};
  expr g_body{expr::functor{"g", {}}};
  expr h_body{expr::functor{"h", {}}};
  expr g_head{expr::functor{"g", {}}};
  expr h_head{expr::functor{"h", {}}};
  expr k_head{expr::functor{"k", {}}};
  expr l_body{expr::functor{"l", {}}};
  expr i_body{expr::functor{"i", {}}};
  expr j_body{expr::functor{"j", {}}};
  expr i_head{expr::functor{"i", {}}};
  expr j_head{expr::functor{"j", {}}};
  expr l_head{expr::functor{"l", {}}};
  initial_goals.push(&goal_f);
  database.push(rule{&f_head, {&g_body, &h_body}});
  database.push(rule{&g_head, {&i_body, &j_body}});
  database.push(rule{&h_head, {&i_body, &j_body}});
  database.push(rule{&k_head, {&i_body, &j_body, &l_body}});
  database.push(rule{&i_head, {}});
  database.push(rule{&j_head, {}});
  database.push(rule{&l_head, {}});

  i_get_resolution_count& get_resolution_count =
      stack.loc.locate<i_get_resolution_count>();

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kDefaultMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_GE(get_resolution_count.get_resolution_count(), kMinResolutions);
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressMultipleAvoidancesSeveralDecisions) {
  // Four duplicate-rule goals; four scripted decisions. Outcome contract only — not call order.
  expr goal_f{expr::functor{"f", {}}};
  expr goal_g{expr::functor{"g", {}}};
  expr goal_h{expr::functor{"h", {}}};
  expr goal_k{expr::functor{"k", {}}};
  expr f0{expr::functor{"f", {}}};
  expr f1{expr::functor{"f", {}}};
  expr g0{expr::functor{"g", {}}};
  expr g1{expr::functor{"g", {}}};
  expr h0{expr::functor{"h", {}}};
  expr h1{expr::functor{"h", {}}};
  expr k0{expr::functor{"k", {}}};
  expr k1{expr::functor{"k", {}}};
  initial_goals.push(&goal_f);
  initial_goals.push(&goal_g);
  initial_goals.push(&goal_h);
  initial_goals.push(&goal_k);
  database.push(rule{&f0, {}});
  database.push(rule{&f1, {}});
  database.push(rule{&g0, {}});
  database.push(rule{&g1, {}});
  database.push(rule{&h0, {}});
  database.push(rule{&h1, {}});
  database.push(rule{&k0, {}});
  database.push(rule{&k1, {}});

  i_make_initial_goal_lineage& make_initial_goal_lineage =
      stack.loc.locate<i_make_initial_goal_lineage>();
  i_make_resolution_lineage& make_resolution_lineage =
      stack.loc.locate<i_make_resolution_lineage>();
  i_get_decision_count& get_decision_count = stack.loc.locate<i_get_decision_count>();

  const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
  const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
  const goal_lineage* gl_h = make_initial_goal_lineage.make(2);
  const goal_lineage* gl_k = make_initial_goal_lineage.make(3);
  const resolution_lineage* rl_f_0 =
      make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
  const resolution_lineage* rl_g_1 =
      make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{3});
  const resolution_lineage* rl_h_0 =
      make_resolution_lineage.make_resolution_lineage(gl_h, rule_id{4});
  const resolution_lineage* rl_k_1 =
      make_resolution_lineage.make_resolution_lineage(gl_k, rule_id{7});

  static constexpr size_t kMaxResolutions = 32;
  static constexpr size_t kExpectedDecisions = 4;

  EXPECT_CALL(stack.decision_generator, generate())
      .WillOnce(Return(rl_f_0))
      .WillOnce(Return(rl_g_1))
      .WillOnce(Return(rl_h_0))
      .WillOnce(Return(rl_k_1));

  simulation simulation{stack.loc, kMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::solved);
  EXPECT_EQ(get_decision_count.count(), kExpectedDecisions);
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsSolvedStressCdclMhuManyGroundHeadsOnSharedVar) {
  expr abc{expr::functor{"abc", {}}};
  expr def{expr::functor{"def", {}}};
  expr ghi{expr::functor{"ghi", {}}};
  expr jkl{expr::functor{"jkl", {}}};
  expr mno{expr::functor{"mno", {}}};
  expr pqr{expr::functor{"pqr", {}}};
  expr stu{expr::functor{"stu", {}}};
  expr _xyz{expr::functor{"xyz", {}}};
  expr f_head0{expr::functor{"f", {}}};
  expr f_head1{expr::functor{"f", {}}};
  expr g1{expr::functor{"g", {&abc, &_xyz, &pqr}}};
  expr g2{expr::functor{"g", {&def, &_xyz, &pqr}}};
  expr g3{expr::functor{"g", {&ghi, &_xyz, &pqr}}};
  expr g4{expr::functor{"g", {&jkl, &_xyz, &pqr}}};
  expr g5{expr::functor{"g", {&mno, &_xyz, &pqr}}};
  expr g_bad{expr::functor{"g", {&ghi, &jkl, &stu}}};
  database.push(rule{&f_head0, {}});
  database.push(rule{&f_head1, {}});
  database.push(rule{&g1, {}});
  database.push(rule{&g2, {}});
  database.push(rule{&g3, {}});
  database.push(rule{&g4, {}});
  database.push(rule{&g5, {}});
  database.push(rule{&g_bad, {}});

  i_make_initial_goal_lineage& make_initial_goal_lineage =
      stack.loc.locate<i_make_initial_goal_lineage>();
  i_make_resolution_lineage& make_resolution_lineage =
      stack.loc.locate<i_make_resolution_lineage>();
  i_learn_avoidance& learn_avoidance = stack.loc.locate<i_learn_avoidance>();
  i_bind_map& bind_map = stack.loc.locate<i_bind_map>();
  i_var_sequencer& seq = stack.loc.locate<i_var_sequencer>();
  i_make_var& make_var = stack.loc.locate<i_make_var>();
  i_make_functor& make_functor = stack.loc.locate<i_make_functor>();

  const goal_lineage* gl_f = make_initial_goal_lineage.make(0);
  const goal_lineage* gl_g = make_initial_goal_lineage.make(1);
  const resolution_lineage* rl_f_0 =
      make_resolution_lineage.make_resolution_lineage(gl_f, rule_id{0});
  const resolution_lineage* rl_g_abc =
      make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{2});
  const resolution_lineage* rl_g_def =
      make_resolution_lineage.make_resolution_lineage(gl_g, rule_id{3});

  learn_avoidance.learn(lemma{{rl_f_0, rl_g_abc}});

  EXPECT_CALL(stack.decision_generator, generate())
      .WillOnce(Return(rl_f_0))
      .WillOnce(Return(rl_g_def));

  simulation simulation{stack.loc, kDefaultMaxResolutions};
  simulation.set_up();
  const expr* var_a = make_var.make(seq.next());
  const expr* var_b = make_var.make(seq.next());
  const expr* var_c = make_var.make(seq.next());
  const expr* xyz = make_functor.make("xyz", {});
  initial_goals.push(make_functor.make("f", {}));
  initial_goals.push(make_functor.make("g", {var_a, var_b, var_c}));

  EXPECT_EQ(simulation.run(), sim_termination::solved);
  const expr::functor& whnf_a = std::get<expr::functor>(bind_map.whnf(var_a)->content);
  const expr::functor& whnf_b = std::get<expr::functor>(bind_map.whnf(var_b)->content);
  const expr::functor& whnf_c = std::get<expr::functor>(bind_map.whnf(var_c)->content);
  EXPECT_EQ(whnf_b.name, "xyz");
  EXPECT_EQ(whnf_c.name, "pqr");
  EXPECT_EQ(whnf_a.name, "def");
  (void)xyz;
  simulation.tear_down();
}

TEST_F(SimIntegrationTest, RunReturnsDepthExceededStressOnDeepLinearChainWithinBudget) {
  static constexpr size_t kMaxResolutions = 8;
  static constexpr size_t kChainDepth = 20;

  expr ground{expr::functor{"abc", {}}};
  std::vector<expr> storage;
  chain_clause_db(database, storage, "chain", kChainDepth, &ground);

  expr goal{expr::functor{"chain0", {&ground}}};
  initial_goals.push(&goal);

  EXPECT_CALL(stack.decision_generator, generate()).Times(0);

  simulation simulation{stack.loc, kMaxResolutions};
  simulation.set_up();
  EXPECT_EQ(simulation.run(), sim_termination::depth_exceeded);
  EXPECT_EQ(stack.loc.locate<i_get_resolution_count>().get_resolution_count(), kMaxResolutions);
  simulation.tear_down();
}
