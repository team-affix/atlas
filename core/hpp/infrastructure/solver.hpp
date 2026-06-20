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
    ISetUpSim& set_up_sim;
    ITearDownSim& tear_down_sim;
    IRunSim& run_sim;
    IGetDecisionCount& get_decision_count;
    IDeriveLemma& derive_decision_lemma;
    IPinResolutionLineage& pin_resolution_lineage;
    ILearnAvoidance& learn_avoidance;
    IEliminationRouter& elimination_router;
};

template<typename ISUS, typename ITDS, typename IRS, typename IGDC, typename IDL,
         typename IPRL, typename ILA, typename IER>
solver<ISUS,ITDS,IRS,IGDC,IDL,IPRL,ILA,IER>::solver(
    ISUS& sus, ITDS& tds, IRS& rs, IGDC& gdc, IDL& dl, IPRL& prl, ILA& la, IER& er)
    : set_up_sim(sus), tear_down_sim(tds), run_sim(rs), get_decision_count(gdc),
      derive_decision_lemma(dl), pin_resolution_lineage(prl), learn_avoidance(la),
      elimination_router(er) {}

template<typename ISUS, typename ITDS, typename IRS, typename IGDC, typename IDL,
         typename IPRL, typename ILA, typename IER>
coroutine<sim_termination, void>
solver<ISUS,ITDS,IRS,IGDC,IDL,IPRL,ILA,IER>::solve() {
    bool refuted = false;
    while (!refuted) {
        set_up_sim.set_up();
        co_yield run_sim.run();
        refuted = get_decision_count.count() == 0;
        const lemma l = derive_decision_lemma.derive_decision_lemma();
        for (const resolution_lineage* rl : l.get_resolutions())
            pin_resolution_lineage.pin(rl);
        tear_down_sim.tear_down();
        auto elim = learn_avoidance.learn(l);
        if (elim.has_value())
            elimination_router.route(elim.value());
    }
}

#endif
