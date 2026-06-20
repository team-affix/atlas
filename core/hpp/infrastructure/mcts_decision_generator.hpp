#ifndef MCTS_DECISION_GENERATOR_HPP
#define MCTS_DECISION_GENERATOR_HPP

#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/mcts_choice.hpp"

template<typename IMakeResolutionLineage, typename ISrtActiveGoals,
         typename IMctsChoose, typename IGetGoalCandidateRuleIds>
struct mcts_decision_generator {
    mcts_decision_generator(IMakeResolutionLineage&, ISrtActiveGoals&,
                            IMctsChoose&, IGetGoalCandidateRuleIds&);
    const resolution_lineage* generate();
private:
    const goal_lineage* choose_goal();
    rule_id choose_candidate(const goal_lineage*);
    IMakeResolutionLineage& make_resolution_lineage_;
    ISrtActiveGoals& srt_active_goals_;
    IMctsChoose& mcts_choose_;
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;
};

template<typename IMRL, typename ISAG, typename IMC, typename IGCRI>
mcts_decision_generator<IMRL, ISAG, IMC, IGCRI>::mcts_decision_generator(
    IMRL& lp, ISAG& sag, IMC& mc, IGCRI& gcr)
    : make_resolution_lineage_(lp), srt_active_goals_(sag),
      mcts_choose_(mc), get_goal_candidate_rule_ids_(gcr) {}

template<typename IMRL, typename ISAG, typename IMC, typename IGCRI>
const resolution_lineage* mcts_decision_generator<IMRL, ISAG, IMC, IGCRI>::generate() {
    const goal_lineage* gl = choose_goal();
    rule_id r = choose_candidate(gl);
    return make_resolution_lineage_.make_resolution_lineage(gl, r);
}

template<typename IMRL, typename ISAG, typename IMC, typename IGCRI>
const goal_lineage* mcts_decision_generator<IMRL, ISAG, IMC, IGCRI>::choose_goal() {
    std::vector<mcts_choice> current;
    auto sm = srt_active_goals_.iterate_root_goals();
    while (true) {
        while (!sm.done()) {
            sm.resume();
            if (!sm.has_yield()) continue;
            current.push_back(sm.consume_yield());
        }
        mcts_choice choice = mcts_choose_.choose(current);
        const goal_lineage* gl = std::get<const goal_lineage*>(choice);
        if (srt_active_goals_.is_active_goal(gl)) return gl;
        current.clear();
        sm = srt_active_goals_.iterate_child_goals(gl);
    }
}

template<typename IMRL, typename ISAG, typename IMC, typename IGCRI>
rule_id mcts_decision_generator<IMRL, ISAG, IMC, IGCRI>::choose_candidate(
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
