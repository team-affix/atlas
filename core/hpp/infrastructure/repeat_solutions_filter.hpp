#ifndef REPEAT_SOLUTIONS_FILTER_HPP
#define REPEAT_SOLUTIONS_FILTER_HPP

#include "value_objects/lemma.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/sim_termination.hpp"

template<typename IRunSim, typename IDeriveResolutionLemma,
         typename IIsRepeatSolution, typename IRememberSolution,
         typename IPinResolutionLineage>
struct repeat_solutions_filter {
    repeat_solutions_filter(IRunSim&, IDeriveResolutionLemma&,
                            IIsRepeatSolution&, IRememberSolution&,
                            IPinResolutionLineage&);
    sim_termination run();
private:
    IRunSim& run_sim_;
    IDeriveResolutionLemma& derive_resolution_lemma_;
    IIsRepeatSolution& is_repeat_solution_;
    IRememberSolution& remember_solution_;
    IPinResolutionLineage& pin_resolution_lineage_;
};

template<typename IRS, typename IDRL, typename IIRS, typename IRMS, typename IPRL>
repeat_solutions_filter<IRS, IDRL, IIRS, IRMS, IPRL>::repeat_solutions_filter(
    IRS& run_sim, IDRL& derive_resolution_lemma,
    IIRS& is_repeat_solution, IRMS& remember_solution,
    IPRL& pin_resolution_lineage)
    : run_sim_(run_sim)
    , derive_resolution_lemma_(derive_resolution_lemma)
    , is_repeat_solution_(is_repeat_solution)
    , remember_solution_(remember_solution)
    , pin_resolution_lineage_(pin_resolution_lineage)
{}

template<typename IRS, typename IDRL, typename IIRS, typename IRMS, typename IPRL>
sim_termination repeat_solutions_filter<IRS, IDRL, IIRS, IRMS, IPRL>::run() {
    const sim_termination term = run_sim_.run();
    if (term != sim_termination::solved)
        return term;
    const lemma l = derive_resolution_lemma_.derive_resolution_lemma();
    if (is_repeat_solution_.is_repeat_solution(l))
        return sim_termination::conflicted;
    for (const resolution_lineage* rl : l.get_resolutions())
        pin_resolution_lineage_.pin(rl);
    remember_solution_.remember_solution(l);
    return sim_termination::solved;
}

#endif
