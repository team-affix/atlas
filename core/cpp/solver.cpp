#include "../hpp/solver.hpp"
#include "../hpp/locator.hpp"

solver::solver() :
    db(locator::locate<database>(locator_keys::inst_database)),
    gl(locator::locate<goals>(locator_keys::inst_goals)),
    t(locator::locate<trail>(locator_keys::inst_trail)),
    vars(locator::locate<sequencer>(locator_keys::inst_var_sequencer)),
    bm(locator::locate<bind_map>(locator_keys::inst_bind_map)),
    ep(),
    lp(),
    max_resolutions(0),
    c(),
    managed_sim(nullptr) {
    locator::push_frame();
    t.push();
}

solver::~solver() {
    t.pop();
    locator::pop_frame();
}

bool solver::operator()(std::optional<resolutions>& soln) {
    soln = std::nullopt;

    // tear down the previous frame and sim, then set up a fresh frame
    t.pop();
    locator::pop_frame();
    managed_sim = nullptr;
    lp.trim();
    locator::push_frame();
    t.push();

    if (c.refuted())
        return false;

    // build and run the simulation for this iteration
    managed_sim = construct_sim();
    bool solved = (*managed_sim)();

    // derived-class post-processing (e.g. MCTS backpropagation)
    terminate(*managed_sim);

    // learn to avoid the exact derivation path taken this iteration;
    // this guarantees we never revisit the same decisions regardless of outcome
    const decisions& ds = managed_sim->get_decisions();
    c.learn(lemma(ds));

    // pin decision lineages so they survive the next lp.trim()
    for (const resolution_lineage* rl : ds)
        lp.pin(rl);

    if (solved)
        soln = managed_sim->get_resolutions();

    // return true while still going; false only when refuted.
    // if this sim produced a solution, always return true so the caller can
    // consume it before the next call detects (or triggers) refutation.
    return solved || !c.refuted();
}
