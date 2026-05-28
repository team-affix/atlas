#include "../../hpp/infrastructure/solver.hpp"

solver::solver(
    i_set_up_sim& set_up_sim,
    i_tear_down_sim& tear_down_sim,
    i_run_sim& run_sim,
    i_get_decision_count& get_decision_count,
    i_learn_avoidance& learn_avoidance,
    i_elimination_router& elimination_router)
    :
    set_up_sim(set_up_sim),
    tear_down_sim(tear_down_sim),
    run_sim(run_sim),
    get_decision_count(get_decision_count),
    learn_avoidance(learn_avoidance),
    elimination_router(elimination_router) {}

solver::~solver() = default;

state_machine<sim_termination> solver::solve() {
    bool refuted = false;
    while (!refuted) {
        set_up_sim.set_up();
        co_yield run_sim.run();
        refuted = get_decision_count.count() == 0;
        tear_down_sim.tear_down();
    }
}
