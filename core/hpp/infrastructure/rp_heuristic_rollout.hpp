#ifndef RP_HEURISTIC_ROLLOUT_HPP
#define RP_HEURISTIC_ROLLOUT_HPP

#include <cstddef>
#include <variant>
#include <vector>
#include "value_objects/mcts_choice.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

template<typename IRolloutChooseGoal, typename IRolloutChooseRule>
struct rp_heuristic_rollout {
    rp_heuristic_rollout(IRolloutChooseGoal&, IRolloutChooseRule&);

    template<typename IGetChoiceCount, typename IGetChoiceAt>
    mcts_choice rollout_choose(const IGetChoiceCount& get_choice_count,
                               const IGetChoiceAt& get_choice_at);

private:
    IRolloutChooseGoal& rollout_choose_goal_;
    IRolloutChooseRule& rollout_choose_rule_;
};

template<typename IRCG, typename IRCR>
rp_heuristic_rollout<IRCG, IRCR>::rp_heuristic_rollout(IRCG& goal, IRCR& rule)
    : rollout_choose_goal_(goal), rollout_choose_rule_(rule) {}

template<typename IRCG, typename IRCR>
template<typename IGetChoiceCount, typename IGetChoiceAt>
mcts_choice rp_heuristic_rollout<IRCG, IRCR>::rollout_choose(
    const IGetChoiceCount& get_choice_count,
    const IGetChoiceAt& get_choice_at) {
    DEBUG_ASSERT(get_choice_count.size() > 0);
    const mcts_choice first = get_choice_at.at(0);
    if (std::holds_alternative<const goal_lineage*>(first)) {
        std::vector<const goal_lineage*> goals;
        goals.reserve(get_choice_count.size());
        for (size_t i = 0; i < get_choice_count.size(); ++i)
            goals.push_back(std::get<const goal_lineage*>(get_choice_at.at(i)));
        return rollout_choose_goal_.rollout_choose_goal(goals);
    }
    std::vector<rule_id> rules;
    rules.reserve(get_choice_count.size());
    for (size_t i = 0; i < get_choice_count.size(); ++i)
        rules.push_back(std::get<rule_id>(get_choice_at.at(i)));
    return rollout_choose_rule_.rollout_choose_rule(rules);
}

#endif
