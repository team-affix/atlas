#ifndef PUD_MODULES_HPP
#define PUD_MODULES_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

// Every registered module by its id, each node reached through the module it belongs to. A module is
// registered after every module it depends on. A module's queries see the axioms of every module it
// depends on, directly or not, dependencies before their dependents, then its own.
template<typename IModule>
struct pud_modules {
    pud_modules();
    void register_module(IModule& data);
    const std::vector<pud_global_node_id>& list_axioms(pud_module_id module) const;
    const std::vector<pud_global_node_id>& list_visible_axioms(pud_module_id module) const;
    pud_global_node_id store_inference(pud_module_id module, pud_inference inference);
    void mark_refuted(pud_global_node_id node);
    const rule& get_axiom(pud_global_node_id axiom) const;
    const pud_inference& get_inference(pud_global_node_id inference) const;
    std::optional<pud_global_node_id> get_parent(pud_global_node_id node) const;
    const std::vector<pud_node_id>& get_children(pud_global_node_id node) const;
    bool check_leaf(pud_global_node_id node) const;
    bool check_refuted(pud_global_node_id node) const;
private:
    struct registration {
        IModule* data;
        std::vector<pud_global_node_id> axioms;
        std::vector<pud_global_node_id> visible_axioms;
    };

    void visit_dependencies(
        pud_module_id module,
        std::unordered_set<pud_module_id>& visited,
        std::vector<pud_module_id>& dependencies_first) const;

    std::unordered_map<pud_module_id, registration> modules_;
};

template<typename IModule>
pud_modules<IModule>::pud_modules()
    : modules_() {}

template<typename IModule>
void pud_modules<IModule>::register_module(IModule& data) {
    const pud_module_id module = data.get_module_id();
    DEBUG_ASSERT(!modules_.contains(module));
    for (pud_module_id dependency : data.get_dependencies())
        DEBUG_ASSERT(modules_.contains(dependency));
    registration registered{.data = &data, .axioms = {}, .visible_axioms = {}};
    for (pud_node_id axiom = 0; axiom < data.count_axioms(); ++axiom)
        registered.axioms.push_back(pud_global_node_id{.module = module, .node = axiom});
    std::unordered_set<pud_module_id> visited{module};
    std::vector<pud_module_id> dependencies_first;
    for (pud_module_id dependency : data.get_dependencies())
        visit_dependencies(dependency, visited, dependencies_first);
    for (pud_module_id seen : dependencies_first) {
        const std::vector<pud_global_node_id>& seen_axioms = modules_.at(seen).axioms;
        registered.visible_axioms.insert(registered.visible_axioms.end(), seen_axioms.begin(), seen_axioms.end());
    }
    registered.visible_axioms.insert(registered.visible_axioms.end(), registered.axioms.begin(), registered.axioms.end());
    modules_.emplace(module, std::move(registered));
}

template<typename IModule>
const std::vector<pud_global_node_id>& pud_modules<IModule>::list_axioms(pud_module_id module) const {
    return modules_.at(module).axioms;
}

template<typename IModule>
const std::vector<pud_global_node_id>& pud_modules<IModule>::list_visible_axioms(pud_module_id module) const {
    return modules_.at(module).visible_axioms;
}

template<typename IModule>
pud_global_node_id pud_modules<IModule>::store_inference(pud_module_id module, pud_inference inference) {
    return pud_global_node_id{.module = module, .node = modules_.at(module).data->store_inference(std::move(inference))};
}

template<typename IModule>
void pud_modules<IModule>::mark_refuted(pud_global_node_id node) {
    modules_.at(node.module).data->mark_refuted(node.node);
}

template<typename IModule>
const rule& pud_modules<IModule>::get_axiom(pud_global_node_id axiom) const {
    return modules_.at(axiom.module).data->get_axiom(axiom.node);
}

template<typename IModule>
const pud_inference& pud_modules<IModule>::get_inference(pud_global_node_id inference) const {
    return modules_.at(inference.module).data->get_inference(inference.node);
}

template<typename IModule>
std::optional<pud_global_node_id> pud_modules<IModule>::get_parent(pud_global_node_id node) const {
    const std::optional<pud_node_id> parent = modules_.at(node.module).data->get_parent(node.node);
    if (!parent.has_value())
        return std::nullopt;
    return pud_global_node_id{.module = node.module, .node = parent.value()};
}

// The children's ids within the node's module.
template<typename IModule>
const std::vector<pud_node_id>& pud_modules<IModule>::get_children(pud_global_node_id node) const {
    return modules_.at(node.module).data->get_children(node.node);
}

template<typename IModule>
bool pud_modules<IModule>::check_leaf(pud_global_node_id node) const {
    return modules_.at(node.module).data->check_leaf(node.node);
}

template<typename IModule>
bool pud_modules<IModule>::check_refuted(pud_global_node_id node) const {
    return modules_.at(node.module).data->check_refuted(node.node);
}

template<typename IModule>
void pud_modules<IModule>::visit_dependencies(
    pud_module_id module,
    std::unordered_set<pud_module_id>& visited,
    std::vector<pud_module_id>& dependencies_first) const {
    if (!visited.insert(module).second)
        return;
    for (pud_module_id dependency : modules_.at(module).data->get_dependencies())
        visit_dependencies(dependency, visited, dependencies_first);
    dependencies_first.push_back(module);
}

#endif
