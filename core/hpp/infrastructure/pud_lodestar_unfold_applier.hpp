#ifndef PUD_LODESTAR_UNFOLD_APPLIER_HPP
#define PUD_LODESTAR_UNFOLD_APPLIER_HPP

#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_lodestar_unfold_outcome.hpp"

// Unfolds one leaf on one goal and carries the change into the frontier and the focus set, mirroring
// what pud_lodestar_module_registrar does for a registered module.
template<typename IUnfold, typename IApplyUnfold, typename IExtendFocus, typename IContainsFocusNode>
struct pud_lodestar_unfold_applier {
    pud_lodestar_unfold_applier(
        IUnfold& unfold,
        IApplyUnfold& apply_unfold,
        IExtendFocus& extend_focus,
        IContainsFocusNode& contains_focus_node);
    pud_lodestar_unfold_outcome unfold(pud_global_node_id leaf, pud_body_goal_id goal);
private:
    IUnfold& unfold_;
    IApplyUnfold& apply_unfold_;
    IExtendFocus& extend_focus_;
    IContainsFocusNode& contains_focus_node_;
};

#define PUD_LODESTAR_UNFOLD_APPLIER_TEMPLATE \
    template<typename IUnfold, typename IApplyUnfold, typename IExtendFocus, typename IContainsFocusNode>
#define PUD_LODESTAR_UNFOLD_APPLIER pud_lodestar_unfold_applier<IUnfold, IApplyUnfold, IExtendFocus, IContainsFocusNode>

PUD_LODESTAR_UNFOLD_APPLIER_TEMPLATE
PUD_LODESTAR_UNFOLD_APPLIER::pud_lodestar_unfold_applier(
    IUnfold& unfold,
    IApplyUnfold& apply_unfold,
    IExtendFocus& extend_focus,
    IContainsFocusNode& contains_focus_node)
    : unfold_(unfold)
    , apply_unfold_(apply_unfold)
    , extend_focus_(extend_focus)
    , contains_focus_node_(contains_focus_node) {}

PUD_LODESTAR_UNFOLD_APPLIER_TEMPLATE
pud_lodestar_unfold_outcome PUD_LODESTAR_UNFOLD_APPLIER::unfold(pud_global_node_id leaf, pud_body_goal_id goal) {
    const pud_frontier_change change = unfold_.unfold(leaf, goal);
    apply_unfold_.apply_unfold(leaf, change);
    extend_focus_.extend(leaf, change.offspring);
    if (contains_focus_node_.contains(leaf))
        return pud_lodestar_unfold_outcome::on_focus;
    return pud_lodestar_unfold_outcome::off_focus;
}

#undef PUD_LODESTAR_UNFOLD_APPLIER
#undef PUD_LODESTAR_UNFOLD_APPLIER_TEMPLATE

#endif
