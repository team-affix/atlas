#ifndef PHOENIX_POLICY_CHOOSE_HPP
#define PHOENIX_POLICY_CHOOSE_HPP

#include "value_objects/mcts_choice.hpp"
#include "debug_assert.hpp"

template<typename ICheckIsRuleChoice, typename IPolicyChooseGoal, typename IPolicyChooseRule>
struct phoenix_policy_choose {
    phoenix_policy_choose(ICheckIsRuleChoice&, IPolicyChooseGoal&, IPolicyChooseRule&);

    template<typename INodeHandle, typename IGetChoiceCount, typename IGetChoiceAt>
    mcts_choice policy_choose(const INodeHandle& node,
                              const IGetChoiceCount& get_choice_count,
                              const IGetChoiceAt& get_choice_at);

private:
    ICheckIsRuleChoice& check_is_rule_choice_;
    IPolicyChooseGoal& policy_choose_goal_;
    IPolicyChooseRule& policy_choose_rule_;
};

template<typename ICR, typename IPCG, typename IPCR>
phoenix_policy_choose<ICR, IPCG, IPCR>::phoenix_policy_choose(
        ICR& check_is_rule_choice, IPCG& policy_choose_goal, IPCR& policy_choose_rule)
    : check_is_rule_choice_(check_is_rule_choice)
    , policy_choose_goal_(policy_choose_goal)
    , policy_choose_rule_(policy_choose_rule) {}

template<typename ICR, typename IPCG, typename IPCR>
template<typename INodeHandle, typename IGetChoiceCount, typename IGetChoiceAt>
mcts_choice phoenix_policy_choose<ICR, IPCG, IPCR>::policy_choose(
        const INodeHandle& node,
        const IGetChoiceCount& get_choice_count,
        const IGetChoiceAt& get_choice_at) {
    DEBUG_ASSERT(get_choice_count.size() > 0);
    if (check_is_rule_choice_.check_is_rule_choice(get_choice_at.at(0)))
        return policy_choose_rule_.policy_choose(node, get_choice_count, get_choice_at);
    return policy_choose_goal_.policy_choose(node, get_choice_count, get_choice_at);
}

#endif
