#ifndef PUD_LODESTAR_SOLVED_LEAF_FINDER_HPP
#define PUD_LODESTAR_SOLVED_LEAF_FINDER_HPP

#include <optional>
#include "value_objects/pud_global_node_id.hpp"

template<typename IGetLiveNodes, typename IContainsFocusNode, typename ICheckRefuted, typename IGetNodeGoals>
struct pud_lodestar_solved_leaf_finder {
    pud_lodestar_solved_leaf_finder(
        IGetLiveNodes& get_live_nodes,
        IContainsFocusNode& contains_focus_node,
        ICheckRefuted& check_refuted,
        IGetNodeGoals& get_node_goals);
    std::optional<pud_global_node_id> find() const;
private:
    IGetLiveNodes& get_live_nodes_;
    IContainsFocusNode& contains_focus_node_;
    ICheckRefuted& check_refuted_;
    IGetNodeGoals& get_node_goals_;
};

#define PUD_LODESTAR_SOLVED_LEAF_FINDER_TEMPLATE \
    template<typename IGetLiveNodes, typename IContainsFocusNode, typename ICheckRefuted, typename IGetNodeGoals>
#define PUD_LODESTAR_SOLVED_LEAF_FINDER \
    pud_lodestar_solved_leaf_finder<IGetLiveNodes, IContainsFocusNode, ICheckRefuted, IGetNodeGoals>

PUD_LODESTAR_SOLVED_LEAF_FINDER_TEMPLATE
PUD_LODESTAR_SOLVED_LEAF_FINDER::pud_lodestar_solved_leaf_finder(
    IGetLiveNodes& get_live_nodes,
    IContainsFocusNode& contains_focus_node,
    ICheckRefuted& check_refuted,
    IGetNodeGoals& get_node_goals)
    : get_live_nodes_(get_live_nodes)
    , contains_focus_node_(contains_focus_node)
    , check_refuted_(check_refuted)
    , get_node_goals_(get_node_goals) {}

PUD_LODESTAR_SOLVED_LEAF_FINDER_TEMPLATE
std::optional<pud_global_node_id> PUD_LODESTAR_SOLVED_LEAF_FINDER::find() const {
    for (const pud_global_node_id leaf : get_live_nodes_.live()) {
        if (!contains_focus_node_.contains(leaf))
            continue;
        if (check_refuted_.check_refuted(leaf))
            continue;
        if (!get_node_goals_.get(leaf).goals.empty())
            continue;
        return leaf;
    }
    return std::nullopt;
}

#undef PUD_LODESTAR_SOLVED_LEAF_FINDER
#undef PUD_LODESTAR_SOLVED_LEAF_FINDER_TEMPLATE

#endif
