#ifndef RUN_SIM_HPP
#define RUN_SIM_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_run_sim.hpp"
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
#include "interfaces/i_activate_initial_goals_and_candidates.hpp"
#include "interfaces/i_record_decision.hpp"
#include "interfaces/i_record_resolution.hpp"

struct run_sim : i_run_sim {
    run_sim(locator& loc, size_t max_resolutions);
    sim_termination run() override;
private:
    const resolution_lineage* next_resolution();

    size_t max_resolutions_;
    i_activate_initial_goals_and_candidates& activate_initial_goals_and_candidates_;
    i_solution_detector& solution_detector_;
    i_conflict_detector& conflict_detector_;
    i_detect_unit_goal& unit_goal_detector_;
    i_push_unit_goal& push_unit_goal_;
    i_pop_unit_goal& pop_unit_goal_;
    i_generate_decision& generate_decision_;
    i_elimination_generator& elimination_generator_;
    i_elimination_router& elimination_router_;
    i_resolver& resolver_;
    i_get_unit_resolution& get_unit_resolution_;
    i_record_decision& record_decision_;
    i_record_resolution& record_resolution_;
};

#endif
