#include "infrastructure/solver.hpp"

solver::solver(locator& loc)
    :
    set_up_sim(loc.locate<i_set_up_sim>()),
    tear_down_sim(loc.locate<i_tear_down_sim>()),
    run_sim(loc.locate<i_run_sim>()),
    get_decision_count(loc.locate<i_get_decision_count>()),
    derive_decision_lemma(loc.locate<i_derive_decision_lemma>()),
    pin_resolution_lineage(loc.locate<i_pin_resolution_lineage>()),
    learn_avoidance(loc.locate<i_learn_avoidance>()),
    elimination_router(loc.locate<i_elimination_router>()) {}

solver::~solver() = default;

coroutine<sim_termination, void> solver::solve() {
    bool refuted = false;
    while (!refuted) {
        // set up sim
        set_up_sim.set_up();

        // run sim
        co_yield run_sim.run();

        refuted = get_decision_count.count() == 0;

        const lemma lemma = derive_decision_lemma.derive_decision_lemma();
        for (const resolution_lineage* rl : lemma.get_resolutions())
            pin_resolution_lineage.pin(rl);

        tear_down_sim.tear_down();

        auto elim = learn_avoidance.learn(lemma);
        if (elim.has_value())
            elimination_router.route(elim.value());
    }
}
