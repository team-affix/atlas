#ifndef SIM_HPP
#define SIM_HPP

#include "interfaces/i_run_sim.hpp"
#include "interfaces/i_set_up_sim.hpp"
#include "interfaces/i_tear_down_sim.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
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
#include "interfaces/i_get_goal_db_rule_ids.hpp"
#include "interfaces/i_candidate_activator.hpp"
#include "interfaces/i_activate_initial_goal.hpp"
#include "interfaces/i_get_initial_goal_count.hpp"
#include "interfaces/i_make_initial_goal_lineage.hpp"
#include "interfaces/i_push_trail_frame.hpp"
#include "interfaces/i_pop_trail_frame.hpp"
#include "interfaces/i_clear_unit_goals.hpp"
#include "interfaces/i_record_decision.hpp"
#include "interfaces/i_record_resolution.hpp"
#include "interfaces/i_clear_recorded_decisions.hpp"
#include "interfaces/i_clear_recorded_resolutions.hpp"
#include "interfaces/i_deactivated_candidate_memory.hpp"
#include "interfaces/i_clear_goal_candidate_rule_ids.hpp"
#include "interfaces/i_clear_goal_exprs.hpp"
#include "interfaces/i_clear_active_goals.hpp"
#include "interfaces/i_clear_candidate_translation_maps.hpp"
#include "interfaces/i_clear_mhu_heads.hpp"
#include "interfaces/i_clear_bindings.hpp"
#include "interfaces/i_trim_unpinned_lineages.hpp"

struct sim
    : i_run_sim
    , i_set_up_sim
    , i_tear_down_sim {
    sim(
        size_t max_resolutions,
        i_push_trail_frame& push_trail_frame,
        i_pop_trail_frame& pop_trail_frame,
        i_get_initial_goal_count& get_initial_goal_count,
        i_activate_initial_goal& activate_initial_goal,
        i_make_initial_goal_lineage& make_initial_goal_lineage,
        i_get_goal_db_rule_ids& get_goal_db_rule_ids,
        i_make_resolution_lineage& make_resolution_lineage,
        i_candidate_activator& candidate_activator,
        i_solution_detector& sd,
        i_conflict_detector& cd,
        i_detect_unit_goal& ugd,
        i_push_unit_goal& push_unit_goal,
        i_pop_unit_goal& pop_unit_goal,
        i_generate_decision& generate_decision,
        i_elimination_generator& eg,
        i_elimination_router& er,
        i_resolver& r,
        i_get_unit_resolution& get_unit_resolution,
        i_record_decision& record_decision,
        i_record_resolution& record_resolution,
        i_clear_unit_goals& clear_unit_goals,
        i_clear_recorded_decisions& clear_recorded_decisions,
        i_clear_recorded_resolutions& clear_recorded_resolutions,
        i_deactivated_candidate_memory& deactivated_candidate_memory,
        i_clear_goal_candidate_rule_ids& clear_goal_candidate_rule_ids,
        i_clear_goal_exprs& clear_goal_exprs,
        i_clear_active_goals& clear_active_goals,
        i_clear_candidate_translation_maps& clear_candidate_translation_maps,
        i_clear_mhu_heads& clear_mhu_heads,
        i_clear_bindings& clear_bindings,
        i_trim_unpinned_lineages& trim_unpinned_lineages);
    void set_up() override;
    sim_termination run() override;
    void tear_down() override;
private:
    const resolution_lineage* next_resolution();
    size_t max_resolutions;
    i_push_trail_frame& push_trail_frame;
    i_pop_trail_frame& pop_trail_frame;
    i_get_initial_goal_count& get_initial_goal_count;
    i_activate_initial_goal& activate_initial_goal;
    i_make_initial_goal_lineage& make_initial_goal_lineage;
    i_get_goal_db_rule_ids& get_goal_db_rule_ids;
    i_make_resolution_lineage& make_resolution_lineage;
    i_candidate_activator& candidate_activator;
    i_solution_detector& sd;
    i_conflict_detector& cd;
    i_detect_unit_goal& ugd;
    i_push_unit_goal& push_unit_goal;
    i_pop_unit_goal& pop_unit_goal;
    i_generate_decision& generate_decision;
    i_elimination_generator& eg;
    i_elimination_router& er;
    i_resolver& r;
    i_get_unit_resolution& get_unit_resolution;
    i_record_decision& record_decision;
    i_record_resolution& record_resolution;
    i_clear_unit_goals& clear_unit_goals;
    i_clear_recorded_decisions& clear_recorded_decisions;
    i_clear_recorded_resolutions& clear_recorded_resolutions;
    i_deactivated_candidate_memory& deactivated_candidate_memory;
    i_clear_goal_candidate_rule_ids& clear_goal_candidate_rule_ids;
    i_clear_goal_exprs& clear_goal_exprs;
    i_clear_active_goals& clear_active_goals;
    i_clear_candidate_translation_maps& clear_candidate_translation_maps;
    i_clear_mhu_heads& clear_mhu_heads;
    i_clear_bindings& clear_bindings;
    i_trim_unpinned_lineages& trim_unpinned_lineages;
};

#endif
