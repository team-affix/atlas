#ifndef SIM_HPP
#define SIM_HPP

#include "../interfaces/i_run_sim.hpp"
#include "../interfaces/i_set_up_sim.hpp"
#include "../interfaces/i_tear_down_sim.hpp"
#include "../interfaces/i_make_resolution_lineage.hpp"
#include "../interfaces/i_solution_detector.hpp"
#include "../interfaces/i_conflict_detector.hpp"
#include "../interfaces/i_detect_unit_goal.hpp"
#include "../interfaces/i_push_unit_goal.hpp"
#include "../interfaces/i_pop_unit_goal.hpp"
#include "../interfaces/i_generate_decision.hpp"
#include "../interfaces/i_elimination_generator.hpp"
#include "../interfaces/i_elimination_router.hpp"
#include "../interfaces/i_resolver.hpp"
#include "../interfaces/i_get_goal_candidate_rules.hpp"
#include "../interfaces/i_get_goal_db_rules.hpp"
#include "../interfaces/i_candidate_activator.hpp"
#include "../interfaces/i_activate_initial_goal.hpp"
#include "../interfaces/i_get_initial_goal_count.hpp"
#include "../interfaces/i_make_initial_goal_lineage.hpp"
#include "../utility/i_trail.hpp"

struct sim
    : i_run_sim
    , i_set_up_sim
    , i_tear_down_sim {
    sim(
        size_t max_resolutions,
        i_trail& trail,
        i_get_initial_goal_count& get_initial_goal_count,
        i_activate_initial_goal& activate_initial_goal,
        i_make_initial_goal_lineage& make_initial_goal_lineage,
        i_get_goal_db_rules& get_goal_db_rules,
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
        i_get_goal_candidate_rules& ggcr);
    void set_up() override;
    sim_termination run() override;
    void tear_down() override;
private:
    const resolution_lineage* next_resolution();
    size_t max_resolutions;
    i_trail& trail;
    i_get_initial_goal_count& get_initial_goal_count;
    i_activate_initial_goal& activate_initial_goal;
    i_make_initial_goal_lineage& make_initial_goal_lineage;
    i_get_goal_db_rules& get_goal_db_rules;
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
    i_get_goal_candidate_rules& ggcr;
};

#endif
