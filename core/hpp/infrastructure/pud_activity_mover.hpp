#ifndef PUD_ACTIVITY_MOVER_HPP
#define PUD_ACTIVITY_MOVER_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"

template<typename IActivateChild, typename IRefuteLeaf, typename IDeactivateNode>
struct pud_activity_mover {
    pud_activity_mover(IActivateChild& activate_child, IRefuteLeaf& refute_leaf, IDeactivateNode& deactivate_node);
    std::vector<pud_global_node_id> move(pud_global_node_id parent, const std::vector<pud_global_node_id>& children);
private:
    IActivateChild& activate_child_;
    IRefuteLeaf& refute_leaf_;
    IDeactivateNode& deactivate_node_;
};

#define PUD_ACTIVITY_MOVER_TEMPLATE template<typename IActivateChild, typename IRefuteLeaf, typename IDeactivateNode>
#define PUD_ACTIVITY_MOVER pud_activity_mover<IActivateChild, IRefuteLeaf, IDeactivateNode>

PUD_ACTIVITY_MOVER_TEMPLATE
PUD_ACTIVITY_MOVER::pud_activity_mover(
    IActivateChild& activate_child, IRefuteLeaf& refute_leaf, IDeactivateNode& deactivate_node)
    : activate_child_(activate_child)
    , refute_leaf_(refute_leaf)
    , deactivate_node_(deactivate_node) {}

PUD_ACTIVITY_MOVER_TEMPLATE
std::vector<pud_global_node_id> PUD_ACTIVITY_MOVER::move(
    pud_global_node_id parent, const std::vector<pud_global_node_id>& children) {
    std::vector<pud_global_node_id> arrived_refuted;
    for (const pud_global_node_id child : children) {
        if (!activate_child_.activate_child(parent, child))
            continue;
        arrived_refuted.push_back(child);
    }
    std::vector<pud_global_node_id> refuted;
    for (const pud_global_node_id child : arrived_refuted) {
        const std::vector<pud_global_node_id> cascaded = refute_leaf_.refute(child);
        refuted.insert(refuted.end(), cascaded.begin(), cascaded.end());
    }
    for (const pud_global_node_id stranded : deactivate_node_.deactivate(parent)) {
        const std::vector<pud_global_node_id> cascaded = refute_leaf_.refute(stranded);
        refuted.insert(refuted.end(), cascaded.begin(), cascaded.end());
    }
    return refuted;
}

#undef PUD_ACTIVITY_MOVER
#undef PUD_ACTIVITY_MOVER_TEMPLATE

#endif
