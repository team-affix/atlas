#ifndef PUD_FOCUS_LEAF_TALLY_HPP
#define PUD_FOCUS_LEAF_TALLY_HPP

#include <algorithm>
#include <utility>
#include <vector>
#include "value_objects/pud_focus_leaf_counts.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"

// Wraps the frontier's mutations and keeps a running count of the live leaves under the focus, and of
// those the unfold allower blocks. Every node a mutation touches is taken out of the count before the
// mutation is forwarded and put back after it, so the count follows exactly what the frontier did.
// Focus membership and the allower's answer never change while a leaf is live.
template<
    typename IApplyRegister,
    typename IApplyUnfold,
    typename IEraseRefuted,
    typename IEraseLeaf,
    typename IGetLiveNodes,
    typename IContainsFocusNode,
    typename IAllowUnfold>
struct pud_focus_leaf_tally {
    pud_focus_leaf_tally(
        IApplyRegister& apply_register,
        IApplyUnfold& apply_unfold,
        IEraseRefuted& erase_refuted,
        IEraseLeaf& erase_leaf,
        IGetLiveNodes& get_live_nodes,
        IContainsFocusNode& contains_focus_node,
        IAllowUnfold& allow_unfold);
    void apply_register(const pud_frontier_change& change);
    void apply_unfold(pud_global_node_id parent, const pud_frontier_change& change);
    void erase_refuted(const std::vector<pud_global_node_id>& refuted);
    void erase(pud_global_node_id node);
    pud_focus_leaf_counts count() const;
private:
    std::vector<pud_global_node_id> list_touched(std::vector<pud_global_node_id> nodes) const;
    void add_counted(const std::vector<pud_global_node_id>& touched);
    void subtract_counted(const std::vector<pud_global_node_id>& touched);

    IApplyRegister& apply_register_;
    IApplyUnfold& apply_unfold_;
    IEraseRefuted& erase_refuted_;
    IEraseLeaf& erase_leaf_;
    IGetLiveNodes& get_live_nodes_;
    IContainsFocusNode& contains_focus_node_;
    IAllowUnfold& allow_unfold_;
    pud_focus_leaf_counts counts_;
};

#define PUD_FOCUS_LEAF_TALLY_TEMPLATE                                                                \
    template<typename IApplyRegister, typename IApplyUnfold, typename IEraseRefuted, typename IEraseLeaf, \
             typename IGetLiveNodes, typename IContainsFocusNode, typename IAllowUnfold>
#define PUD_FOCUS_LEAF_TALLY                                                                         \
    pud_focus_leaf_tally<IApplyRegister, IApplyUnfold, IEraseRefuted, IEraseLeaf, IGetLiveNodes,         \
        IContainsFocusNode, IAllowUnfold>

PUD_FOCUS_LEAF_TALLY_TEMPLATE
PUD_FOCUS_LEAF_TALLY::pud_focus_leaf_tally(
    IApplyRegister& apply_register,
    IApplyUnfold& apply_unfold,
    IEraseRefuted& erase_refuted,
    IEraseLeaf& erase_leaf,
    IGetLiveNodes& get_live_nodes,
    IContainsFocusNode& contains_focus_node,
    IAllowUnfold& allow_unfold)
    : apply_register_(apply_register)
    , apply_unfold_(apply_unfold)
    , erase_refuted_(erase_refuted)
    , erase_leaf_(erase_leaf)
    , get_live_nodes_(get_live_nodes)
    , contains_focus_node_(contains_focus_node)
    , allow_unfold_(allow_unfold)
    , counts_{.live = 0, .blocked = 0} {}

PUD_FOCUS_LEAF_TALLY_TEMPLATE
void PUD_FOCUS_LEAF_TALLY::apply_register(const pud_frontier_change& change) {
    std::vector<pud_global_node_id> nodes = change.offspring;
    nodes.insert(nodes.end(), change.refuted.begin(), change.refuted.end());
    const std::vector<pud_global_node_id> touched = list_touched(std::move(nodes));
    subtract_counted(touched);
    apply_register_.apply_register(change);
    add_counted(touched);
}

PUD_FOCUS_LEAF_TALLY_TEMPLATE
void PUD_FOCUS_LEAF_TALLY::apply_unfold(pud_global_node_id parent, const pud_frontier_change& change) {
    std::vector<pud_global_node_id> nodes = change.offspring;
    nodes.insert(nodes.end(), change.refuted.begin(), change.refuted.end());
    nodes.push_back(parent);
    const std::vector<pud_global_node_id> touched = list_touched(std::move(nodes));
    subtract_counted(touched);
    apply_unfold_.apply_unfold(parent, change);
    add_counted(touched);
}

PUD_FOCUS_LEAF_TALLY_TEMPLATE
void PUD_FOCUS_LEAF_TALLY::erase_refuted(const std::vector<pud_global_node_id>& refuted) {
    const std::vector<pud_global_node_id> touched = list_touched(refuted);
    subtract_counted(touched);
    erase_refuted_.erase_refuted(refuted);
    add_counted(touched);
}

PUD_FOCUS_LEAF_TALLY_TEMPLATE
void PUD_FOCUS_LEAF_TALLY::erase(pud_global_node_id node) {
    const std::vector<pud_global_node_id> touched{node};
    subtract_counted(touched);
    erase_leaf_.erase(node);
    add_counted(touched);
}

PUD_FOCUS_LEAF_TALLY_TEMPLATE
pud_focus_leaf_counts PUD_FOCUS_LEAF_TALLY::count() const {
    return counts_;
}

PUD_FOCUS_LEAF_TALLY_TEMPLATE
std::vector<pud_global_node_id> PUD_FOCUS_LEAF_TALLY::list_touched(std::vector<pud_global_node_id> nodes) const {
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    return nodes;
}

PUD_FOCUS_LEAF_TALLY_TEMPLATE
void PUD_FOCUS_LEAF_TALLY::add_counted(const std::vector<pud_global_node_id>& touched) {
    for (const pud_global_node_id node : touched) {
        if (!get_live_nodes_.live().contains(node))
            continue;
        if (!contains_focus_node_.contains(node))
            continue;
        ++counts_.live;
        if (!allow_unfold_.allow(node))
            ++counts_.blocked;
    }
}

PUD_FOCUS_LEAF_TALLY_TEMPLATE
void PUD_FOCUS_LEAF_TALLY::subtract_counted(const std::vector<pud_global_node_id>& touched) {
    for (const pud_global_node_id node : touched) {
        if (!get_live_nodes_.live().contains(node))
            continue;
        if (!contains_focus_node_.contains(node))
            continue;
        --counts_.live;
        if (!allow_unfold_.allow(node))
            --counts_.blocked;
    }
}

#undef PUD_FOCUS_LEAF_TALLY
#undef PUD_FOCUS_LEAF_TALLY_TEMPLATE

#endif
