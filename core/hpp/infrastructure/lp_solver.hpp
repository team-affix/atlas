#ifndef LP_SOLVER_HPP
#define LP_SOLVER_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"

// Restart solver for the lazy-propagation stack. It differs from solver only in
// how the avoidance is learned: learn() takes no argument, because the store
// builds the avoidance from the full decision set it has been accumulating, and
// it must run before tear-down, because tear-down clears that decision set.
//
// The lemma is still derived, but only to pin lineages against the trim() at the
// end of tear-down. Pinning walks parents transitively, and every ancestor a
// lemma strips is an ancestor of a retained leaf, so the whole decision set
// survives.
template<typename ISetUpSim, typename ITearDownSim, typename IRunSim,
         typename IGetDecisionCount, typename IDeriveLemma,
         typename IPinResolutionLineage, typename ILearnAvoidance,
         typename IEliminationRouter>
struct lp_solver {
    lp_solver(ISetUpSim&, ITearDownSim&, IRunSim&, IGetDecisionCount&, IDeriveLemma&,
              IPinResolutionLineage&, ILearnAvoidance&, IEliminationRouter&);
    coroutine<sim_termination, void> solve();
private:
    ISetUpSim& set_up_sim_;
    ITearDownSim& tear_down_sim_;
    IRunSim& run_sim_;
    IGetDecisionCount& get_decision_count_;
    IDeriveLemma& derive_decision_lemma_;
    IPinResolutionLineage& pin_resolution_lineage_;
    ILearnAvoidance& learn_avoidance_;
    IEliminationRouter& elimination_router_;
};

template<typename ISUS, typename ITDS, typename IRS, typename IGDC, typename IDL,
         typename IPRL, typename ILA, typename IER>
lp_solver<ISUS,ITDS,IRS,IGDC,IDL,IPRL,ILA,IER>::lp_solver(
    ISUS& sus, ITDS& tds, IRS& rs, IGDC& gdc, IDL& dl, IPRL& prl, ILA& la, IER& er)
    : set_up_sim_(sus), tear_down_sim_(tds), run_sim_(rs), get_decision_count_(gdc),
      derive_decision_lemma_(dl), pin_resolution_lineage_(prl), learn_avoidance_(la),
      elimination_router_(er) {}

template<typename ISUS, typename ITDS, typename IRS, typename IGDC, typename IDL,
         typename IPRL, typename ILA, typename IER>
coroutine<sim_termination, void>
lp_solver<ISUS,ITDS,IRS,IGDC,IDL,IPRL,ILA,IER>::solve() {
    bool refuted = false;
    while (!refuted) {
        set_up_sim_.set_up();
        co_yield run_sim_.run();
        refuted = get_decision_count_.count() == 0;
        const lemma l = derive_decision_lemma_.derive_decision_lemma();
        for (const resolution_lineage* rl : l.get_resolutions())
            pin_resolution_lineage_.pin(rl);
        auto elim = learn_avoidance_.learn();
        tear_down_sim_.tear_down();
        if (elim.has_value())
            elimination_router_.route(elim.value());
    }
}

#endif
