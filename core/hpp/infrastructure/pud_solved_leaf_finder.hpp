#ifndef PUD_SOLVED_LEAF_FINDER_HPP
#define PUD_SOLVED_LEAF_FINDER_HPP

#include <optional>
#include "value_objects/pud_global_node_id.hpp"

template<typename IGetLiveNodes, typename IContainsFocusNode, typename IIsNodeRefuted, typename IGetNodeGoals>
struct pud_solved_leaf_finder {
    pud_solved_leaf_finder(
        IGetLiveNodes& get_live_nodes,
        IContainsFocusNode& contains_focus_node,
        IIsNodeRefuted& is_refuted,
        IGetNodeGoals& get_node_goals);
    std::optional<pud_global_node_id> find() const;
private:
    IGetLiveNodes& get_live_nodes_;
    IContainsFocusNode& contains_focus_node_;
    IIsNodeRefuted& is_refuted_;
    IGetNodeGoals& get_node_goals_;
};

#define PUD_SOLVED_LEAF_FINDER_TEMPLATE \
    template<typename IGetLiveNodes, typename IContainsFocusNode, typename IIsNodeRefuted, typename IGetNodeGoals>
#define PUD_SOLVED_LEAF_FINDER \
    pud_solved_leaf_finder<IGetLiveNodes, IContainsFocusNode, IIsNodeRefuted, IGetNodeGoals>

PUD_SOLVED_LEAF_FINDER_TEMPLATE
PUD_SOLVED_LEAF_FINDER::pud_solved_leaf_finder(
    IGetLiveNodes& get_live_nodes,
    IContainsFocusNode& contains_focus_node,
    IIsNodeRefuted& is_refuted,
    IGetNodeGoals& get_node_goals)
    : get_live_nodes_(get_live_nodes)
    , contains_focus_node_(contains_focus_node)
    , is_refuted_(is_refuted)
    , get_node_goals_(get_node_goals) {}

PUD_SOLVED_LEAF_FINDER_TEMPLATE
std::optional<pud_global_node_id> PUD_SOLVED_LEAF_FINDER::find() const {
    for (const pud_global_node_id leaf : get_live_nodes_.live()) {
        if (!contains_focus_node_.contains(leaf))
            continue;
        if (is_refuted_.is_refuted(leaf))
            continue;
        if (!get_node_goals_.get(leaf).goals.empty())
            continue;
        return leaf;
    }
    return std::nullopt;
}

#undef PUD_SOLVED_LEAF_FINDER
#undef PUD_SOLVED_LEAF_FINDER_TEMPLATE

#endif
