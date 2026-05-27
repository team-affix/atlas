#ifndef SOLVER_HPP
#define SOLVER_HPP

#include "../interfaces/i_solver.hpp"
#include "../interfaces/i_set_up_sim.hpp"
#include "../interfaces/i_tear_down_sim.hpp"
#include "../interfaces/i_run_sim.hpp"
#include "../interfaces/i_cdcl_elimination_generator.hpp"
#include "../interfaces/i_elimination_router.hpp"

struct solver : i_solver {
    solver(
        i_set_up_sim& set_up_sim,
        i_tear_down_sim& tear_down_sim,
        i_run_sim& run_sim,
        i_cdcl_elimination_generator& cdcl_elimination_generator,
        i_elimination_router& elimination_router);
    virtual ~solver();
    state_machine<solver_yield> solve() override;
private:
    i_set_up_sim& set_up_sim;
    i_tear_down_sim& tear_down_sim;
    i_run_sim& run_sim;
    i_cdcl_elimination_generator& cdcl_elimination_generator;
    i_elimination_router& elimination_router;
};

#endif
