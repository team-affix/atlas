#ifndef RUN_SIM_HPP
#define RUN_SIM_HPP

#include <cstddef>
#include "value_objects/sim_termination.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/elimination_result.hpp"

template<typename IActivateInitialGoals, typename ISolutionDetector, typename IConflictDetector,
         typename IUnitGoalDetector, typename IPushUnitGoal, typename IPopUnitGoal,
         typename IGenerateDecision, typename IEliminationGenerator, typename IEliminationRouter,
         typename IResolver, typename IGetUnitResolution, typename IRecordDecisionResolution,
         typename IRecordUnitResolution, typename IGetResolutionCount>
struct run_sim {
    run_sim(IActivateInitialGoals&, ISolutionDetector&, IConflictDetector&,
            IUnitGoalDetector&, IPushUnitGoal&, IPopUnitGoal&,
            IGenerateDecision&, IEliminationGenerator&, IEliminationRouter&,
            IResolver&, IGetUnitResolution&, IRecordDecisionResolution&,
            IRecordUnitResolution&, IGetResolutionCount&, size_t max_resolutions);
    sim_termination run();
private:
    const resolution_lineage* next_resolution();

    size_t max_resolutions_;
    IActivateInitialGoals& activate_initial_goals_and_candidates_;
    ISolutionDetector& solution_detector_;
    IConflictDetector& conflict_detector_;
    IUnitGoalDetector& unit_goal_detector_;
    IPushUnitGoal& push_unit_goal_;
    IPopUnitGoal& pop_unit_goal_;
    IGenerateDecision& generate_decision_;
    IEliminationGenerator& elimination_generator_;
    IEliminationRouter& elimination_router_;
    IResolver& resolver_;
    IGetUnitResolution& get_unit_resolution_;
    // Two independent recorder slots: a caller may back them with a single object
    // (both methods on one recorder) or route decision and unit resolutions to
    // two different objects.
    IRecordDecisionResolution& record_decision_resolution_;
    IRecordUnitResolution& record_unit_resolution_;
    IGetResolutionCount& get_resolution_count_;
};

// --- method definitions ---

template<typename IAI, typename ISD, typename ICD, typename IUGD,
         typename IPUG, typename IPOG, typename IGD, typename IEG,
         typename IER, typename IR, typename IGUR, typename IRDR,
         typename IRUR, typename IGRC>
run_sim<IAI,ISD,ICD,IUGD,IPUG,IPOG,IGD,IEG,IER,IR,IGUR,IRDR,IRUR,IGRC>::run_sim(
    IAI& ai, ISD& sd, ICD& cd, IUGD& ugd, IPUG& pug, IPOG& pog,
    IGD& gd, IEG& eg, IER& er, IR& r, IGUR& gur, IRDR& rdr,
    IRUR& rur, IGRC& grc, size_t max_resolutions)
    : max_resolutions_(max_resolutions),
      activate_initial_goals_and_candidates_(ai),
      solution_detector_(sd),
      conflict_detector_(cd),
      unit_goal_detector_(ugd),
      push_unit_goal_(pug),
      pop_unit_goal_(pog),
      generate_decision_(gd),
      elimination_generator_(eg),
      elimination_router_(er),
      resolver_(r),
      get_unit_resolution_(gur),
      record_decision_resolution_(rdr),
      record_unit_resolution_(rur),
      get_resolution_count_(grc) {}

template<typename IAI, typename ISD, typename ICD, typename IUGD,
         typename IPUG, typename IPOG, typename IGD, typename IEG,
         typename IER, typename IR, typename IGUR, typename IRDR,
         typename IRUR, typename IGRC>
sim_termination run_sim<IAI,ISD,ICD,IUGD,IPUG,IPOG,IGD,IEG,IER,IR,IGUR,IRDR,IRUR,IGRC>::run() {
    if (!activate_initial_goals_and_candidates_.activate_initial_goals_and_candidates())
        return sim_termination::conflicted;

    while (get_resolution_count_.get_resolution_count() < max_resolutions_) {
        if (solution_detector_.detect())
            return sim_termination::solved;
        const resolution_lineage* rl = next_resolution();
        auto eliminations = elimination_generator_.constrain(rl);
        while (!eliminations.done()) {
            eliminations.resume();
            if (!eliminations.has_yield())
                continue;
            const resolution_lineage* elim_rl = eliminations.consume_yield();
            if (elimination_router_.route(elim_rl) != elimination_result::eliminated)
                continue;
            const goal_lineage* gl = elim_rl->parent;
            if (conflict_detector_.detect(gl))
                return sim_termination::conflicted;
            if (unit_goal_detector_.detect(gl))
                push_unit_goal_.push(gl);
        }
        if (!resolver_.resolve(rl))
            return sim_termination::conflicted;
    }
    return sim_termination::depth_exceeded;
}

template<typename IAI, typename ISD, typename ICD, typename IUGD,
         typename IPUG, typename IPOG, typename IGD, typename IEG,
         typename IER, typename IR, typename IGUR, typename IRDR,
         typename IRUR, typename IGRC>
const resolution_lineage*
run_sim<IAI,ISD,ICD,IUGD,IPUG,IPOG,IGD,IEG,IER,IR,IGUR,IRDR,IRUR,IGRC>::next_resolution() {
    const resolution_lineage* rl;
    auto maybe_gl = pop_unit_goal_.pop();
    if (!maybe_gl.has_value()) {
        rl = generate_decision_.generate();
        record_decision_resolution_.record_decision_resolution(rl);
    } else {
        rl = get_unit_resolution_.get(maybe_gl.value());
        record_unit_resolution_.record_unit_resolution(rl);
    }
    return rl;
}

#endif
