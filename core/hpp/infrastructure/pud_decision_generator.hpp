#ifndef PUD_DECISION_GENERATOR_HPP
#define PUD_DECISION_GENERATOR_HPP

#include <set>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_decision.hpp"
#include "value_objects/pud_node_id.hpp"

template<typename IChooseUnfoldLeafRule, typename IChooseUnfoldSubgoal>
struct pud_decision_generator {
    pud_decision_generator(
        IChooseUnfoldLeafRule& choose_unfold_leaf_rule,
        IChooseUnfoldSubgoal& choose_unfold_subgoal);
    pud_decision generate(const std::set<pud_node_id>& open_leaves);
private:
    IChooseUnfoldLeafRule& choose_unfold_leaf_rule_;
    IChooseUnfoldSubgoal& choose_unfold_subgoal_;
};

template<typename IChooseUnfoldLeafRule, typename IChooseUnfoldSubgoal>
pud_decision_generator<IChooseUnfoldLeafRule, IChooseUnfoldSubgoal>::pud_decision_generator(
    IChooseUnfoldLeafRule& choose_unfold_leaf_rule,
    IChooseUnfoldSubgoal& choose_unfold_subgoal)
    : choose_unfold_leaf_rule_(choose_unfold_leaf_rule)
    , choose_unfold_subgoal_(choose_unfold_subgoal) {}

template<typename IChooseUnfoldLeafRule, typename IChooseUnfoldSubgoal>
pud_decision pud_decision_generator<IChooseUnfoldLeafRule, IChooseUnfoldSubgoal>::generate(
    const std::set<pud_node_id>& open_leaves) {
    const pud_node_id leaf = choose_unfold_leaf_rule_.choose_rule(open_leaves);
    const pud_body_goal_id goal = choose_unfold_subgoal_.choose_subgoal(leaf);
    return pud_decision{leaf, goal};
}

#endif
