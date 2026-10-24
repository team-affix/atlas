#ifndef PUD_RUNTIME_HPP
#define PUD_RUNTIME_HPP

#include "infrastructure/pud_manifest.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"

// The runtime over any number of modules, each module owned by the caller. A module is registered
// whole, after every module it depends on, and is put on the frontier as it is registered. Any live
// leaf of any module can then be unfolded on one of its goals. Every expression the runtime makes goes
// into the caller's pool.
template<typename IModule, typename IExprPool>
struct pud_runtime {
    pud_runtime(IExprPool& expr_pool);
    pud_frontier_change register_module(IModule& module);
    pud_frontier_change unfold(pud_global_node_id leaf, pud_body_goal_id goal);
private:
    pud_manifest<IModule, IExprPool> manifest_;
};

template<typename IModule, typename IExprPool>
pud_runtime<IModule, IExprPool>::pud_runtime(IExprPool& expr_pool)
    : manifest_(expr_pool) {}

template<typename IModule, typename IExprPool>
pud_frontier_change pud_runtime<IModule, IExprPool>::register_module(IModule& module) {
    manifest_.modules_.register_module(module);
    return manifest_.module_activator_.activate_module(module.get_module_id());
}

template<typename IModule, typename IExprPool>
pud_frontier_change pud_runtime<IModule, IExprPool>::unfold(pud_global_node_id leaf, pud_body_goal_id goal) {
    return manifest_.unfolder_.unfold(leaf, goal);
}

#endif
