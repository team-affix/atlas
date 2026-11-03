#ifndef PUD_UNFOLD_APPLIER_HPP
#define PUD_UNFOLD_APPLIER_HPP

#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_unfold_outcome.hpp"

// Unfolds one leaf on one goal and carries the change into the frontier, the node depths and the focus
// set, mirroring what pud_module_loader does for a registered module.
template<
    typename IUnfold,
    typename IApplyUnfold,
    typename IExtendNodeDepths,
    typename IExtendFocus,
    typename IContainsFocusNode>
struct pud_unfold_applier {
    pud_unfold_applier(
        IUnfold& unfold,
        IApplyUnfold& apply_unfold,
        IExtendNodeDepths& extend_node_depths,
        IExtendFocus& extend_focus,
        IContainsFocusNode& contains_focus_node);
    pud_unfold_outcome unfold(pud_global_node_id leaf, pud_body_goal_id goal);
private:
    IUnfold& unfold_;
    IApplyUnfold& apply_unfold_;
    IExtendNodeDepths& extend_node_depths_;
    IExtendFocus& extend_focus_;
    IContainsFocusNode& contains_focus_node_;
};

#define PUD_UNFOLD_APPLIER_TEMPLATE                                                                 \
    template<typename IUnfold, typename IApplyUnfold, typename IExtendNodeDepths, typename IExtendFocus, \
             typename IContainsFocusNode>
#define PUD_UNFOLD_APPLIER \
    pud_unfold_applier<IUnfold, IApplyUnfold, IExtendNodeDepths, IExtendFocus, IContainsFocusNode>

PUD_UNFOLD_APPLIER_TEMPLATE
PUD_UNFOLD_APPLIER::pud_unfold_applier(
    IUnfold& unfold,
    IApplyUnfold& apply_unfold,
    IExtendNodeDepths& extend_node_depths,
    IExtendFocus& extend_focus,
    IContainsFocusNode& contains_focus_node)
    : unfold_(unfold)
    , apply_unfold_(apply_unfold)
    , extend_node_depths_(extend_node_depths)
    , extend_focus_(extend_focus)
    , contains_focus_node_(contains_focus_node) {}

PUD_UNFOLD_APPLIER_TEMPLATE
pud_unfold_outcome PUD_UNFOLD_APPLIER::unfold(pud_global_node_id leaf, pud_body_goal_id goal) {
    const pud_frontier_change change = unfold_.unfold(leaf, goal);
    apply_unfold_.apply_unfold(leaf, change);
    extend_node_depths_.extend(leaf, change.offspring);
    extend_focus_.extend(leaf, change.offspring);
    if (contains_focus_node_.contains(leaf))
        return pud_unfold_outcome::on_focus;
    return pud_unfold_outcome::off_focus;
}

#undef PUD_UNFOLD_APPLIER
#undef PUD_UNFOLD_APPLIER_TEMPLATE

#endif
