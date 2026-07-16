#ifndef SUBGOALS_ACTIVATOR_HPP
#define SUBGOALS_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IMakeGoalLineage, typename IGoalActivator,
         typename IGetRule, typename IActivateGoalCandidates>
struct subgoals_activator {
    subgoals_activator(IMakeGoalLineage&, IGoalActivator&, IGetRule&, IActivateGoalCandidates&);
    bool activate_subgoals_and_candidates(const resolution_lineage*);
private:
    IMakeGoalLineage& make_goal_lineage_;
    IGoalActivator& goal_activator_;
    IGetRule& get_rule_;
    IActivateGoalCandidates& activate_goal_candidates_;
};

template<typename IMGL, typename IGA, typename IGR, typename IGCA>
subgoals_activator<IMGL, IGA, IGR, IGCA>::subgoals_activator(
    IMGL& lp, IGA& ga, IGR& db, IGCA& gca)
    : make_goal_lineage_(lp), goal_activator_(ga), get_rule_(db), activate_goal_candidates_(gca) {}

template<typename IMGL, typename IGA, typename IGR, typename IGCA>
bool subgoals_activator<IMGL, IGA, IGR, IGCA>::activate_subgoals_and_candidates(
    const resolution_lineage* rl) {
    const rule* rule = get_rule_.get_rule(rl->idx);
    for (size_t body_idx = 0; body_idx < rule->body.size(); ++body_idx) {
        const goal_lineage* gl = make_goal_lineage_.make_goal_lineage(rl, body_idx);
        goal_activator_.activate(gl);
        if (!activate_goal_candidates_.activate_goal_candidates(gl))
            return false;
    }
    return true;
}

#endif
