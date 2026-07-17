#ifndef MCTS_DECISION_GENERATOR_HPP
#define MCTS_DECISION_GENERATOR_HPP

#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/mcts_choice.hpp"

template<typename IMakeResolutionLineage, typename IIterateRootGoals, typename IIsActiveGoal,
         typename IIterateChildGoals, typename IMctsChoose, typename IGetGoalCandidateRuleIds>
struct mcts_decision_generator {
    mcts_decision_generator(IMakeResolutionLineage&, IIterateRootGoals&, IIsActiveGoal&,
                            IIterateChildGoals&, IMctsChoose&, IGetGoalCandidateRuleIds&);
    const resolution_lineage* generate();
private:
    const goal_lineage* choose_goal();
    rule_id choose_candidate(const goal_lineage*);
    IMakeResolutionLineage& make_resolution_lineage_;
    IIterateRootGoals& iterate_root_goals_;
    IIsActiveGoal& is_active_goal_;
    IIterateChildGoals& iterate_child_goals_;
    IMctsChoose& mcts_choose_;
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;
};

template<typename IMRL, typename IIRG, typename IIAG, typename IICG, typename IMC, typename IGCRI>
mcts_decision_generator<IMRL, IIRG, IIAG, IICG, IMC, IGCRI>::mcts_decision_generator(
    IMRL& lp, IIRG& irg, IIAG& iag, IICG& icg, IMC& mc, IGCRI& gcr)
    : make_resolution_lineage_(lp), iterate_root_goals_(irg), is_active_goal_(iag),
      iterate_child_goals_(icg), mcts_choose_(mc), get_goal_candidate_rule_ids_(gcr) {}

template<typename IMRL, typename IIRG, typename IIAG, typename IICG, typename IMC, typename IGCRI>
const resolution_lineage*
mcts_decision_generator<IMRL, IIRG, IIAG, IICG, IMC, IGCRI>::generate() {
    const goal_lineage* gl = choose_goal();
    rule_id r = choose_candidate(gl);
    return make_resolution_lineage_.make_resolution_lineage(gl, r);
}

template<typename IMRL, typename IIRG, typename IIAG, typename IICG, typename IMC, typename IGCRI>
const goal_lineage*
mcts_decision_generator<IMRL, IIRG, IIAG, IICG, IMC, IGCRI>::choose_goal() {
    std::vector<mcts_choice> current;
    auto sm = iterate_root_goals_.iterate_root_goals();
    while (true) {
        while (!sm.done()) {
            sm.resume();
            if (!sm.has_yield()) continue;
            current.push_back(sm.consume_yield());
        }
        mcts_choice choice = mcts_choose_.choose(current);
        const goal_lineage* gl = std::get<const goal_lineage*>(choice);
        if (is_active_goal_.is_active_goal(gl)) return gl;
        current.clear();
        sm = iterate_child_goals_.iterate_child_goals(gl);
    }
}

template<typename IMRL, typename IIRG, typename IIAG, typename IICG, typename IMC, typename IGCRI>
rule_id mcts_decision_generator<IMRL, IIRG, IIAG, IICG, IMC, IGCRI>::choose_candidate(
    const goal_lineage* gl) {
    auto& rule_ids = get_goal_candidate_rule_ids_.get(gl);
    std::vector<mcts_choice> candidates;
    candidates.reserve(rule_ids.size());
    auto sm = rule_ids.iterate();
    while (!sm.done()) {
        sm.resume();
        if (!sm.has_yield()) continue;
        candidates.push_back(sm.consume_yield());
    }
    return std::get<rule_id>(mcts_choose_.choose(candidates));
}

#endif
