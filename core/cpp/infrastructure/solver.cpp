#include "../../hpp/infrastructure/solver.hpp"

solver::solver(
    i_set_up_sim& set_up_sim,
    i_tear_down_sim& tear_down_sim,
    i_run_sim& run_sim,
    i_get_decision_count& get_decision_count,
    i_derive_decision_lemma& derive_decision_lemma,
    i_learn_avoidance& learn_avoidance,
    i_elimination_router& elimination_router)
    :
    set_up_sim(set_up_sim),
    tear_down_sim(tear_down_sim),
    run_sim(run_sim),
    get_decision_count(get_decision_count),
    derive_decision_lemma(derive_decision_lemma),
    learn_avoidance(learn_avoidance),
    elimination_router(elimination_router) {}

solver::~solver() = default;

state_machine<sim_termination> solver::solve() {
    bool refuted = false;
    while (!refuted) {
        // set up sim
        set_up_sim.set_up();

        // run sim
        co_yield run_sim.run();

        // check if refuted
        refuted = get_decision_count.count() == 0;

        // learn avoidance and route elimination
        auto elim = learn_avoidance.learn(derive_decision_lemma.derive());
        if (elim.has_value())
            elimination_router.route(elim.value());

        // tear down sim
        tear_down_sim.tear_down();
    }
}
