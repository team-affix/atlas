#ifndef SOLVER_HPP
#define SOLVER_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/sim_termination.hpp"
#include "value_objects/lemma.hpp"

template<typename ISetUpSim, typename ITearDownSim, typename IRunSim,
         typename IGetDecisionCount, typename IDeriveLemma,
         typename IPinResolutionLineage, typename ILearnAvoidance,
         typename IEliminationRouter>
struct solver {
    solver(ISetUpSim&, ITearDownSim&, IRunSim&, IGetDecisionCount&, IDeriveLemma&,
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
solver<ISUS,ITDS,IRS,IGDC,IDL,IPRL,ILA,IER>::solver(
    ISUS& sus, ITDS& tds, IRS& rs, IGDC& gdc, IDL& dl, IPRL& prl, ILA& la, IER& er)
    : set_up_sim_(sus), tear_down_sim_(tds), run_sim_(rs), get_decision_count_(gdc), derive_decision_lemma_(dl), pin_resolution_lineage_(prl), learn_avoidance_(la), elimination_router_(er) {}

template<typename ISUS, typename ITDS, typename IRS, typename IGDC, typename IDL,
         typename IPRL, typename ILA, typename IER>
coroutine<sim_termination, void>
solver<ISUS,ITDS,IRS,IGDC,IDL,IPRL,ILA,IER>::solve() {
    bool refuted = false;
    while (!refuted) {
        set_up_sim_.set_up();
        co_yield run_sim_.run();
        refuted = get_decision_count_.count() == 0;
        const lemma l = derive_decision_lemma_.derive_decision_lemma();
        for (const resolution_lineage* rl : l.get_resolutions())
            pin_resolution_lineage_.pin(rl);
        tear_down_sim_.tear_down();
        auto elim = learn_avoidance_.learn(l);
        if (elim.has_value())
            elimination_router_.route(elim.value());
    }
}

#endif
