#ifndef PUD_CONTROLLER_HPP
#define PUD_CONTROLLER_HPP

#include "infrastructure/pud_manifest.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"

// The controller over any number of modules, each module's data owned by the caller. A module is
// registered whole, after every module it depends on, and is put on the frontier as it is registered.
// Any live leaf of any module can then be expanded on one of its goals. Every expression the
// controller makes goes into the caller's pool.
template<typename IModuleData, typename IExprPool>
struct pud_controller {
    pud_controller(IExprPool& expr_pool);
    pud_frontier_change register_module(IModuleData& data);
    pud_frontier_change expand(pud_global_node_id leaf, pud_body_goal_id goal);
private:
    pud_manifest<IModuleData, IExprPool> manifest_;
};

template<typename IModuleData, typename IExprPool>
pud_controller<IModuleData, IExprPool>::pud_controller(IExprPool& expr_pool)
    : manifest_(expr_pool) {}

template<typename IModuleData, typename IExprPool>
pud_frontier_change pud_controller<IModuleData, IExprPool>::register_module(IModuleData& data) {
    manifest_.modules_.register_module(data);
    return manifest_.module_activator_.activate_module(data.get_module_id());
}

template<typename IModuleData, typename IExprPool>
pud_frontier_change pud_controller<IModuleData, IExprPool>::expand(pud_global_node_id leaf, pud_body_goal_id goal) {
    return manifest_.expander_.expand(leaf, goal);
}

#endif
