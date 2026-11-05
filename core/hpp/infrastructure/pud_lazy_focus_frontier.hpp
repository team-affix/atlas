#ifndef PUD_LAZY_FOCUS_FRONTIER_HPP
#define PUD_LAZY_FOCUS_FRONTIER_HPP

#include <set>
#include <vector>
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"

// The live leaves under the focus, and those of them that are solved, kept from frontier changes. Leaves of other
// trees are not kept at all.
template<typename IContainsFocusNode, typename IGetNodeGoals>
struct pud_lazy_focus_frontier {
    pud_lazy_focus_frontier(IContainsFocusNode& contains_focus_node, IGetNodeGoals& get_node_goals);
    void apply_register(const pud_frontier_change& change);
    void apply_unfold(pud_global_node_id parent, const pud_frontier_change& change);
    void erase_refuted(const std::vector<pud_global_node_id>& refuted);
    void erase(pud_global_node_id node);
    const std::set<pud_global_node_id>& live() const;
    const std::set<pud_global_node_id>& solved() const;
private:
    void insert_focus_offspring(const std::vector<pud_global_node_id>& offspring);
    void erase_nodes(const std::vector<pud_global_node_id>& nodes);

    IContainsFocusNode& contains_focus_node_;
    IGetNodeGoals& get_node_goals_;
    std::set<pud_global_node_id> live_;
    std::set<pud_global_node_id> solved_;
};

#define PUD_LAZY_FOCUS_FRONTIER_TEMPLATE template<typename IContainsFocusNode, typename IGetNodeGoals>
#define PUD_LAZY_FOCUS_FRONTIER          pud_lazy_focus_frontier<IContainsFocusNode, IGetNodeGoals>

PUD_LAZY_FOCUS_FRONTIER_TEMPLATE
PUD_LAZY_FOCUS_FRONTIER::pud_lazy_focus_frontier(IContainsFocusNode& contains_focus_node, IGetNodeGoals& get_node_goals)
    : contains_focus_node_(contains_focus_node)
    , get_node_goals_(get_node_goals)
    , live_()
    , solved_() {}

PUD_LAZY_FOCUS_FRONTIER_TEMPLATE
void PUD_LAZY_FOCUS_FRONTIER::apply_register(const pud_frontier_change& change) {
    insert_focus_offspring(change.offspring);
    erase_nodes(change.refuted);
}

PUD_LAZY_FOCUS_FRONTIER_TEMPLATE
void PUD_LAZY_FOCUS_FRONTIER::apply_unfold(pud_global_node_id parent, const pud_frontier_change& change) {
    live_.erase(parent);
    solved_.erase(parent);
    insert_focus_offspring(change.offspring);
    erase_nodes(change.refuted);
}

PUD_LAZY_FOCUS_FRONTIER_TEMPLATE
void PUD_LAZY_FOCUS_FRONTIER::erase_refuted(const std::vector<pud_global_node_id>& refuted) {
    erase_nodes(refuted);
}

PUD_LAZY_FOCUS_FRONTIER_TEMPLATE
void PUD_LAZY_FOCUS_FRONTIER::erase(pud_global_node_id node) {
    live_.erase(node);
    solved_.erase(node);
}

PUD_LAZY_FOCUS_FRONTIER_TEMPLATE
const std::set<pud_global_node_id>& PUD_LAZY_FOCUS_FRONTIER::live() const {
    return live_;
}

PUD_LAZY_FOCUS_FRONTIER_TEMPLATE
const std::set<pud_global_node_id>& PUD_LAZY_FOCUS_FRONTIER::solved() const {
    return solved_;
}

PUD_LAZY_FOCUS_FRONTIER_TEMPLATE
void PUD_LAZY_FOCUS_FRONTIER::insert_focus_offspring(const std::vector<pud_global_node_id>& offspring) {
    for (const pud_global_node_id node : offspring) {
        if (!contains_focus_node_.contains(node))
            continue;
        live_.insert(node);
        if (get_node_goals_.get(node).goals.empty())
            solved_.insert(node);
    }
}

PUD_LAZY_FOCUS_FRONTIER_TEMPLATE
void PUD_LAZY_FOCUS_FRONTIER::erase_nodes(const std::vector<pud_global_node_id>& nodes) {
    for (const pud_global_node_id node : nodes) {
        live_.erase(node);
        solved_.erase(node);
    }
}

#undef PUD_LAZY_FOCUS_FRONTIER
#undef PUD_LAZY_FOCUS_FRONTIER_TEMPLATE

#endif
