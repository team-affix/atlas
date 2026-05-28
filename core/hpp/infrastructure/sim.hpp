#ifndef SIM_HPP
#define SIM_HPP

#include "../interfaces/i_run_sim.hpp"
#include "../interfaces/i_set_up_sim.hpp"
#include "../interfaces/i_tear_down_sim.hpp"
#include "../interfaces/i_make_resolution_lineage.hpp"
#include "../interfaces/i_solution_detector.hpp"
#include "../interfaces/i_conflict_detector.hpp"
#include "../interfaces/i_unit_goal_detector.hpp"
#include "../interfaces/i_push_unit_goal.hpp"
#include "../interfaces/i_pop_unit_goal.hpp"
#include "../interfaces/i_decision_generator.hpp"
#include "../interfaces/i_elimination_generator.hpp"
#include "../interfaces/i_elimination_router.hpp"
#include "../interfaces/i_resolver.hpp"
#include "../interfaces/i_get_goal_candidate_rules.hpp"

struct sim
    : i_run_sim
    , i_set_up_sim
    , i_tear_down_sim {
    sim(
        size_t max_resolutions,
        i_make_resolution_lineage& make_resolution_lineage,
        i_solution_detector& sd,
        i_conflict_detector& cd,
        i_unit_goal_detector& ugd,
        i_push_unit_goal& push_unit_goal,
        i_pop_unit_goal& pop_unit_goal,
        i_decision_generator& dg,
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
    i_make_resolution_lineage& make_resolution_lineage;
    i_solution_detector& sd;
    i_conflict_detector& cd;
    i_unit_goal_detector& ugd;
    i_push_unit_goal& push_unit_goal;
    i_pop_unit_goal& pop_unit_goal;
    i_decision_generator& dg;
    i_elimination_generator& eg;
    i_elimination_router& er;
    i_resolver& r;
    i_get_goal_candidate_rules& ggcr;
};

#endif
