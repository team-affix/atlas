#ifndef PUD_FOCUS_FRONTIER_HPP
#define PUD_FOCUS_FRONTIER_HPP

#include <set>
#include <vector>
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"

// Wraps the frontier's mutations and keeps its own set of the live leaves under the focus, and of those the
// solved ones (no open goals), so readers that only want focus leaves never walk the whole frontier. Every
// mutation is forwarded, then mirrored here for the nodes in the focus. A leaf joins the focus before it goes
// live and never leaves it, and its goals are fixed once it is live.
template<
    typename IApplyRegister,
    typename IApplyUnfold,
    typename IEraseRefuted,
    typename IEraseLeaf,
    typename IContainsFocusNode,
    typename IGetNodeGoals>
struct pud_focus_frontier {
    pud_focus_frontier(
        IApplyRegister& apply_register,
        IApplyUnfold& apply_unfold,
        IEraseRefuted& erase_refuted,
        IEraseLeaf& erase_leaf,
        IContainsFocusNode& contains_focus_node,
        IGetNodeGoals& get_node_goals);
    void apply_register(const pud_frontier_change& change);
    void apply_unfold(pud_global_node_id parent, const pud_frontier_change& change);
    void erase_refuted(const std::vector<pud_global_node_id>& refuted);
    void erase(pud_global_node_id node);
    const std::set<pud_global_node_id>& live() const;
    const std::set<pud_global_node_id>& solved() const;
private:
    void insert_focus_offspring(const std::vector<pud_global_node_id>& offspring);
    void erase_nodes(const std::vector<pud_global_node_id>& nodes);

    IApplyRegister& apply_register_;
    IApplyUnfold& apply_unfold_;
    IEraseRefuted& erase_refuted_;
    IEraseLeaf& erase_leaf_;
    IContainsFocusNode& contains_focus_node_;
    IGetNodeGoals& get_node_goals_;
    std::set<pud_global_node_id> live_;
    std::set<pud_global_node_id> solved_;
};

#define PUD_FOCUS_FRONTIER_TEMPLATE                                                                  \
    template<typename IApplyRegister, typename IApplyUnfold, typename IEraseRefuted, typename IEraseLeaf, \
             typename IContainsFocusNode, typename IGetNodeGoals>
#define PUD_FOCUS_FRONTIER \
    pud_focus_frontier<IApplyRegister, IApplyUnfold, IEraseRefuted, IEraseLeaf, IContainsFocusNode, IGetNodeGoals>

PUD_FOCUS_FRONTIER_TEMPLATE
PUD_FOCUS_FRONTIER::pud_focus_frontier(
    IApplyRegister& apply_register,
    IApplyUnfold& apply_unfold,
    IEraseRefuted& erase_refuted,
    IEraseLeaf& erase_leaf,
    IContainsFocusNode& contains_focus_node,
    IGetNodeGoals& get_node_goals)
    : apply_register_(apply_register)
    , apply_unfold_(apply_unfold)
    , erase_refuted_(erase_refuted)
    , erase_leaf_(erase_leaf)
    , contains_focus_node_(contains_focus_node)
    , get_node_goals_(get_node_goals)
    , live_()
    , solved_() {}

PUD_FOCUS_FRONTIER_TEMPLATE
void PUD_FOCUS_FRONTIER::apply_register(const pud_frontier_change& change) {
    apply_register_.apply_register(change);
    insert_focus_offspring(change.offspring);
    erase_nodes(change.refuted);
}

PUD_FOCUS_FRONTIER_TEMPLATE
void PUD_FOCUS_FRONTIER::apply_unfold(pud_global_node_id parent, const pud_frontier_change& change) {
    apply_unfold_.apply_unfold(parent, change);
    live_.erase(parent);
    solved_.erase(parent);
    insert_focus_offspring(change.offspring);
    erase_nodes(change.refuted);
}

PUD_FOCUS_FRONTIER_TEMPLATE
void PUD_FOCUS_FRONTIER::erase_refuted(const std::vector<pud_global_node_id>& refuted) {
    erase_refuted_.erase_refuted(refuted);
    erase_nodes(refuted);
}

PUD_FOCUS_FRONTIER_TEMPLATE
void PUD_FOCUS_FRONTIER::erase(pud_global_node_id node) {
    erase_leaf_.erase(node);
    live_.erase(node);
    solved_.erase(node);
}

PUD_FOCUS_FRONTIER_TEMPLATE
const std::set<pud_global_node_id>& PUD_FOCUS_FRONTIER::live() const {
    return live_;
}

PUD_FOCUS_FRONTIER_TEMPLATE
const std::set<pud_global_node_id>& PUD_FOCUS_FRONTIER::solved() const {
    return solved_;
}

PUD_FOCUS_FRONTIER_TEMPLATE
void PUD_FOCUS_FRONTIER::insert_focus_offspring(const std::vector<pud_global_node_id>& offspring) {
    for (const pud_global_node_id node : offspring) {
        if (!contains_focus_node_.contains(node))
            continue;
        live_.insert(node);
        if (get_node_goals_.get(node).goals.empty())
            solved_.insert(node);
    }
}

PUD_FOCUS_FRONTIER_TEMPLATE
void PUD_FOCUS_FRONTIER::erase_nodes(const std::vector<pud_global_node_id>& nodes) {
    for (const pud_global_node_id node : nodes) {
        live_.erase(node);
        solved_.erase(node);
    }
}

#undef PUD_FOCUS_FRONTIER
#undef PUD_FOCUS_FRONTIER_TEMPLATE

#endif
