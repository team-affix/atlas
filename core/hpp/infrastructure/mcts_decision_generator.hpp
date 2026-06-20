#ifndef MCTS_DECISION_GENERATOR_HPP
#define MCTS_DECISION_GENERATOR_HPP

#include <vector>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/mcts_choice.hpp"

template<typename ILineagePool, typename ISrtActiveGoals,
         typename IMctsChoose, typename IGoalCandidateRules>
struct mcts_decision_generator {
    mcts_decision_generator(ILineagePool&, ISrtActiveGoals&,
                            IMctsChoose&, IGoalCandidateRules&);
    const resolution_lineage* generate();
private:
    const goal_lineage* choose_goal();
    rule_id choose_candidate(const goal_lineage*);
    ILineagePool& make_resolution_lineage_;
    ISrtActiveGoals& srt_active_goals_;
    IMctsChoose& mcts_choose_;
    IGoalCandidateRules& get_goal_candidate_rule_ids_;
};

template<typename ILP, typename ISAG, typename IMC, typename IGCR>
mcts_decision_generator<ILP, ISAG, IMC, IGCR>::mcts_decision_generator(
    ILP& lp, ISAG& sag, IMC& mc, IGCR& gcr)
    : make_resolution_lineage_(lp), srt_active_goals_(sag),
      mcts_choose_(mc), get_goal_candidate_rule_ids_(gcr) {}

template<typename ILP, typename ISAG, typename IMC, typename IGCR>
const resolution_lineage* mcts_decision_generator<ILP, ISAG, IMC, IGCR>::generate() {
    const goal_lineage* gl = choose_goal();
    rule_id r = choose_candidate(gl);
    return make_resolution_lineage_.make_resolution_lineage(gl, r);
}

template<typename ILP, typename ISAG, typename IMC, typename IGCR>
const goal_lineage* mcts_decision_generator<ILP, ISAG, IMC, IGCR>::choose_goal() {
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

template<typename ILP, typename ISAG, typename IMC, typename IGCR>
rule_id mcts_decision_generator<ILP, ISAG, IMC, IGCR>::choose_candidate(
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
