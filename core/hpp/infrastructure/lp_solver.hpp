#ifndef LP_SOLVER_HPP
#define LP_SOLVER_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"

// Restart solver for the lazy-propagation stack. It differs from solver in two
// ways.
//
// learn() takes no argument and returns nothing: the store builds the avoidance
// from the full decision set it has been accumulating and delivers it to the
// root frame's mailbox, and it must run before tear-down, because tear-down
// clears that decision set.
//
// The root is entered here, which is what makes it an ordinary frame rather than
// a special case. Entry drains the root's mailbox and stages the eliminations
// the root has proven -- eliminations that tear-down undid even though the
// proofs behind them stand. Nothing descends into the root, so this is the only
// place that re-applies them. Ordering is load-bearing: entry must follow
// set_up(), so the routed eliminations land in the backlog frame tear-down will
// roll back, and must precede run(), so they are in place before the initial
// goals are activated.
//
// The lemma is still derived, but only to pin lineages against the trim() at the
// end of tear-down. Pinning walks parents transitively, and every ancestor a
// lemma strips is an ancestor of a retained leaf, so the whole decision set
// survives.
template<typename ISetUpSim, typename ITearDownSim, typename IRunSim,
         typename IGetDecisionCount, typename IDeriveLemma,
         typename IPinResolutionLineage, typename ILearnAvoidance,
         typename IEnterDecisionFrame, typename IFlushEliminations,
         typename IEliminationRouter>
struct lp_solver {
    lp_solver(ISetUpSim&, ITearDownSim&, IRunSim&, IGetDecisionCount&, IDeriveLemma&,
              IPinResolutionLineage&, ILearnAvoidance&, IEnterDecisionFrame&,
              IFlushEliminations&, IEliminationRouter&);
    coroutine<sim_termination, void> solve();
private:
    ISetUpSim& set_up_sim_;
    ITearDownSim& tear_down_sim_;
    IRunSim& run_sim_;
    IGetDecisionCount& get_decision_count_;
    IDeriveLemma& derive_decision_lemma_;
    IPinResolutionLineage& pin_resolution_lineage_;
    ILearnAvoidance& learn_avoidance_;
    IEnterDecisionFrame& enter_decision_frame_;
    IFlushEliminations& flush_eliminations_;
    IEliminationRouter& elimination_router_;
};

template<typename ISUS, typename ITDS, typename IRS, typename IGDC, typename IDL,
         typename IPRL, typename ILA, typename IEDF, typename IFE, typename IER>
lp_solver<ISUS,ITDS,IRS,IGDC,IDL,IPRL,ILA,IEDF,IFE,IER>::lp_solver(
    ISUS& sus, ITDS& tds, IRS& rs, IGDC& gdc, IDL& dl, IPRL& prl, ILA& la, IEDF& edf,
    IFE& fe, IER& er)
    : set_up_sim_(sus), tear_down_sim_(tds), run_sim_(rs), get_decision_count_(gdc),
      derive_decision_lemma_(dl), pin_resolution_lineage_(prl), learn_avoidance_(la),
      enter_decision_frame_(edf), flush_eliminations_(fe), elimination_router_(er) {}

template<typename ISUS, typename ITDS, typename IRS, typename IGDC, typename IDL,
         typename IPRL, typename ILA, typename IEDF, typename IFE, typename IER>
coroutine<sim_termination, void>
lp_solver<ISUS,ITDS,IRS,IGDC,IDL,IPRL,ILA,IEDF,IFE,IER>::solve() {
    bool refuted = false;
    while (!refuted) {
        set_up_sim_.set_up();
        enter_decision_frame_.enter();
        // No goal is active yet, so the router backlogs each of these; the
        // candidate activator then skips them as run_sim activates the initial
        // goals, which gets conflict detection and unit-goal pushing for free.
        auto elims = flush_eliminations_.flush();
        while (!elims.done()) {
            elims.resume();
            if (elims.has_yield())
                elimination_router_.route(elims.consume_yield());
        }
        co_yield run_sim_.run();
        refuted = get_decision_count_.count() == 0;
        const lemma l = derive_decision_lemma_.derive_decision_lemma();
        for (const resolution_lineage* rl : l.get_resolutions())
            pin_resolution_lineage_.pin(rl);
        learn_avoidance_.learn();
        tear_down_sim_.tear_down();
    }
}

#endif
