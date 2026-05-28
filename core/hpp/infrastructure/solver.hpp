#ifndef SOLVER_HPP
#define SOLVER_HPP

#include "../interfaces/i_solve.hpp"
#include "../interfaces/i_set_up_sim.hpp"
#include "../interfaces/i_tear_down_sim.hpp"
#include "../interfaces/i_run_sim.hpp"
#include "../interfaces/i_learn_avoidance.hpp"
#include "../interfaces/i_elimination_router.hpp"
#include "core/hpp/interfaces/i_get_decision_count.hpp"

struct solver : i_solve {
    solver(
        i_set_up_sim& set_up_sim,
        i_tear_down_sim& tear_down_sim,
        i_run_sim& run_sim,
        i_get_decision_count& get_decision_count,
        i_learn_avoidance& learn_avoidance,
        i_elimination_router& elimination_router);
    ~solver() override;
    state_machine<sim_termination> solve() override;
private:
    i_set_up_sim& set_up_sim;
    i_tear_down_sim& tear_down_sim;
    i_run_sim& run_sim;
    i_get_decision_count& get_decision_count;
    i_learn_avoidance& learn_avoidance;
    i_elimination_router& elimination_router;
};

#endif
