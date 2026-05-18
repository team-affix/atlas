#ifndef SIM_HPP
#define SIM_HPP

#include "../interfaces/i_sim.hpp"
#include "../interfaces/i_solution_detector.hpp"
#include "../interfaces/i_conflict_detector.hpp"
#include "../interfaces/i_unit_goal_detector.hpp"
#include "../interfaces/i_unit_goals.hpp"
#include "../interfaces/i_decision_generator.hpp"
#include "../interfaces/i_elimination_generator.hpp"
#include "../interfaces/i_elimination_router.hpp"
#include "../interfaces/i_resolver.hpp"
#include "../interfaces/i_goal_candidates_acceptor.hpp"
#include "../interfaces/i_goal_candidates_expr_visitor_factory.hpp"

struct sim : i_sim {
    sim(
        size_t max_resolutions,
        i_solution_detector& sd,
        i_conflict_detector& cd,
        i_unit_goal_detector& ugd,
        i_unit_goals& ug,
        i_decision_generator& dg,
        i_elimination_generator& eg,
        i_elimination_router& er,
        i_resolver& r,
        i_goal_candidates_acceptor& gca,
        i_goal_candidates_extractor_visitor_factory& gcevf);
    sim_termination run() override;
private:
    const resolution_lineage* next_resolution();
    size_t max_resolutions;
    i_solution_detector& sd;
    i_conflict_detector& cd;
    i_unit_goal_detector& ugd;
    i_unit_goals& ug;
    i_decision_generator& dg;
    i_elimination_generator& eg;
    i_elimination_router& er;
    i_resolver& r;
    i_goal_candidates_acceptor& gca;
    i_goal_candidates_extractor_visitor_factory& gcevf;
};

#endif
