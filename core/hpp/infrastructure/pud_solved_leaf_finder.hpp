#ifndef PUD_SOLVED_LEAF_FINDER_HPP
#define PUD_SOLVED_LEAF_FINDER_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"

template<typename IGetLiveNodes, typename IContainsFocusNode, typename IIsNodeRefuted, typename IGetNodeGoals>
struct pud_solved_leaf_finder {
    pud_solved_leaf_finder(
        IGetLiveNodes& get_live_nodes,
        IContainsFocusNode& contains_focus_node,
        IIsNodeRefuted& is_refuted,
        IGetNodeGoals& get_node_goals);
    std::optional<pud_global_node_id> find() const;
    std::vector<pud_global_node_id> list() const;
private:
    bool is_solved_focus_leaf(pud_global_node_id leaf) const;

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
        if (is_solved_focus_leaf(leaf))
            return leaf;
    }
    return std::nullopt;
}

PUD_SOLVED_LEAF_FINDER_TEMPLATE
std::vector<pud_global_node_id> PUD_SOLVED_LEAF_FINDER::list() const {
    std::vector<pud_global_node_id> solved;
    for (const pud_global_node_id leaf : get_live_nodes_.live()) {
        if (is_solved_focus_leaf(leaf))
            solved.push_back(leaf);
    }
    return solved;
}

PUD_SOLVED_LEAF_FINDER_TEMPLATE
bool PUD_SOLVED_LEAF_FINDER::is_solved_focus_leaf(pud_global_node_id leaf) const {
    if (!contains_focus_node_.contains(leaf))
        return false;
    if (is_refuted_.is_refuted(leaf))
        return false;
    return get_node_goals_.get(leaf).goals.empty();
}

#undef PUD_SOLVED_LEAF_FINDER
#undef PUD_SOLVED_LEAF_FINDER_TEMPLATE

#endif
