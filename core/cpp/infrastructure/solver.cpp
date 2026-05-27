#include "../../hpp/infrastructure/solver.hpp"

solver::solver(
    i_set_up_sim& set_up_sim,
    i_tear_down_sim& tear_down_sim,
    i_run_sim& run_sim,
    i_cdcl_elimination_generator& cdcl_elimination_generator,
    i_elimination_router& elimination_router)
    :
    set_up_sim(set_up_sim),
    tear_down_sim(tear_down_sim),
    run_sim(run_sim),
    cdcl_elimination_generator(cdcl_elimination_generator),
    elimination_router(elimination_router) {}

solver::~solver() = default;

state_machine<sim_termination> solver::solve() {
    bool refuted = false;
    while (!refuted) {
        set_up_sim.set_up();
        auto r = run_sim.run();
        co_yield r;
        tear_down_sim.tear_down();
    }
}
