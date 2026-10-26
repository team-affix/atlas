#ifndef PUD_RUNTIME_HPP
#define PUD_RUNTIME_HPP

#include "infrastructure/pud_manifest.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_unnormalized_rule_information.hpp"

template<typename IModule, typename IExprPool>
struct pud_runtime {
    pud_runtime(IExprPool& expr_pool);
    pud_frontier_change register_module(IModule& module);
    pud_frontier_change unfold(pud_global_node_id leaf, pud_body_goal_id goal);
    pud_unnormalized_rule_information get_rule_information(pud_global_node_id node) const;
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

template<typename IModule, typename IExprPool>
pud_unnormalized_rule_information pud_runtime<IModule, IExprPool>::get_rule_information(pud_global_node_id node) const {
    return pud_unnormalized_rule_information{
        .head = manifest_.modules_.get_axiom(manifest_.node_axioms_.get(node)).head,
        .body = manifest_.node_goals_.get(node).goals,
        .bindings = ubs_bind_map{manifest_.active_states_.get(node).bindings},
    };
}

#endif
