#ifndef REFERENCE_PUD_INFERENCE_VIOLATIONS_HPP
#define REFERENCE_PUD_INFERENCE_VIOLATIONS_HPP

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include "infrastructure/pud_module.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "reference_pud_goal_list.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

inline void reference_pud_inference_violations_collect_vars(const expr* skeleton, std::vector<uint32_t>& variables) {
    if (const expr::var* variable = std::get_if<expr::var>(&skeleton->content)) {
        variables.push_back(variable->index);
        return;
    }
    const expr::functor& term = std::get<expr::functor>(skeleton->content);
    for (const expr* arg : term.args)
        reference_pud_inference_violations_collect_vars(arg, variables);
}

inline uint32_t reference_pud_inference_violations_lvc(const pud_module& module, pud_node_id node) {
    if (node < module.count_axioms())
        return module.get_axiom(node).var_count;
    const pud_inference& inference = module.get_inference(node);
    return reference_pud_inference_violations_lvc(module, inference.parent) + inference.var_count;
}

inline ubs_bind_map reference_pud_inference_violations_bindings(const pud_module& module, pud_node_id node) {
    if (node < module.count_axioms())
        return ubs_bind_map{pud_ubs_state{}};
    std::vector<pud_node_id> path;
    for (pud_node_id current = node; current >= module.count_axioms();) {
        path.push_back(current);
        current = module.get_inference(current).parent;
    }
    std::reverse(path.begin(), path.end());
    ubs_bind_map bind_map{pud_ubs_state{}};
    for (pud_node_id inference_node : path) {
        for (const pud_substitution& substitution : module.get_inference(inference_node).substitutions) {
            const ubs_class target = bind_map.find(substitution.var_idx);
            if (const expr::var* value_var = std::get_if<expr::var>(&substitution.value->content)) {
                bind_map.equate(target, bind_map.find(value_var->index));
                continue;
            }
            bind_map.instantiate(target, moduled_functor{std::get_if<expr::functor>(&(substitution.value)->content), 0, 0});
        }
    }
    return bind_map;
}

inline void reference_pud_inference_violations_visit_node(
    const pud_module& module,
    pud_node_id node,
    std::vector<std::string>& violations) {
    const std::vector<pud_node_id>& children = module.get_children(node);
    if (children.size() >= 2) {
        const pud_body_goal_id expected_call_site = module.get_inference(children.front()).call_site;
        for (size_t child_index = 1; child_index < children.size(); ++child_index) {
            const pud_inference& sibling = module.get_inference(children.at(child_index));
            if (sibling.call_site == expected_call_site)
                continue;
            violations.push_back(
                "inference node " + std::to_string(children.at(child_index))
                + ": sibling call site " + std::to_string(sibling.call_site)
                + " differs from " + std::to_string(expected_call_site) + " on parent "
                + std::to_string(node));
        }
    }
    for (pud_node_id child : children) {
        const pud_inference& inference = module.get_inference(child);
        const pud_node_id parent = inference.parent;
        const pud_goal_list parent_goals = reference_pud_goal_list(module, parent);
        if (parent_goals.goals.find(inference.call_site) == nullptr) {
            violations.push_back(
                "inference node " + std::to_string(child) + ": call site "
                + std::to_string(inference.call_site) + " is not an open goal of parent "
                + std::to_string(parent));
        }
        const uint32_t parent_lvc = reference_pud_inference_violations_lvc(module, parent);
        const ubs_bind_map parent_bindings = reference_pud_inference_violations_bindings(module, parent);
        std::vector<uint32_t> substituted_variables;
        for (const pud_substitution& substitution : inference.substitutions) {
            if (substitution.var_idx >= parent_lvc) {
                violations.push_back(
                    "inference node " + std::to_string(child) + ": substitution var_idx "
                    + std::to_string(substitution.var_idx) + " is not less than parent lvc "
                    + std::to_string(parent_lvc));
            }
            const ubs_class variable_class = parent_bindings.find(substitution.var_idx);
            const bool unbound_representative =
                variable_class.rep == substitution.var_idx && !variable_class.value.has_value();
            if (!unbound_representative) {
                violations.push_back(
                    "inference node " + std::to_string(child) + ": substitution var_idx "
                    + std::to_string(substitution.var_idx)
                    + " is not an unbound representative in the parent bindings");
            }
            substituted_variables.push_back(substitution.var_idx);
        }
        std::sort(substituted_variables.begin(), substituted_variables.end());
        for (size_t index = 1; index < substituted_variables.size(); ++index) {
            if (substituted_variables.at(index) != substituted_variables.at(index - 1))
                continue;
            violations.push_back(
                "inference node " + std::to_string(child) + ": variable "
                + std::to_string(substituted_variables.at(index)) + " is substituted twice");
            break;
        }
        const uint32_t node_lvc = reference_pud_inference_violations_lvc(module, child);
        for (const pud_substitution& substitution : inference.substitutions) {
            std::vector<uint32_t> value_variables;
            reference_pud_inference_violations_collect_vars(substitution.value, value_variables);
            for (uint32_t variable : value_variables) {
                if (variable < node_lvc)
                    continue;
                violations.push_back(
                    "inference node " + std::to_string(child) + ": substitution value uses variable "
                    + std::to_string(variable) + " not less than node lvc " + std::to_string(node_lvc));
            }
        }
        for (const expr* body_goal : inference.body_goals) {
            std::vector<uint32_t> goal_variables;
            reference_pud_inference_violations_collect_vars(body_goal, goal_variables);
            for (uint32_t variable : goal_variables) {
                if (variable < node_lvc)
                    continue;
                violations.push_back(
                    "inference node " + std::to_string(child) + ": body goal uses variable "
                    + std::to_string(variable) + " not less than node lvc " + std::to_string(node_lvc));
            }
        }
        reference_pud_inference_violations_visit_node(module, child, violations);
    }
}

inline std::vector<std::string> reference_pud_inference_violations(const pud_module& module) {
    std::vector<std::string> violations;
    for (pud_node_id axiom = 0; axiom < module.count_axioms(); ++axiom) {
        const rule& axiom_rule = module.get_axiom(axiom);
        std::vector<uint32_t> head_variables;
        reference_pud_inference_violations_collect_vars(axiom_rule.head, head_variables);
        for (uint32_t variable : head_variables) {
            if (variable < axiom_rule.var_count)
                continue;
            violations.push_back(
                "axiom " + std::to_string(axiom) + ": head uses variable " + std::to_string(variable)
                + " not less than var_count " + std::to_string(axiom_rule.var_count));
        }
        for (const expr* body_goal : axiom_rule.body) {
            std::vector<uint32_t> body_variables;
            reference_pud_inference_violations_collect_vars(body_goal, body_variables);
            for (uint32_t variable : body_variables) {
                if (variable < axiom_rule.var_count)
                    continue;
                violations.push_back(
                    "axiom " + std::to_string(axiom) + ": body uses variable " + std::to_string(variable)
                    + " not less than var_count " + std::to_string(axiom_rule.var_count));
            }
        }
        reference_pud_inference_violations_visit_node(module, axiom, violations);
    }
    return violations;
}

#endif
