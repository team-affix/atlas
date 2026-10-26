#include "infrastructure/pud_module.hpp"
#include <utility>
#include "debug_assert.hpp"

pud_module::pud_module(pud_module_id module_id, std::vector<pud_module_id> dependencies, std::vector<rule> axioms)
    : module_id_(module_id)
    , dependencies_(std::move(dependencies))
    , axioms_(std::move(axioms))
    , inferences_()
    , children_(axioms_.size())
    , refuted_(axioms_.size(), false) {}

pud_module_id pud_module::get_module_id() const {
    return module_id_;
}

const std::vector<pud_module_id>& pud_module::get_dependencies() const {
    return dependencies_;
}

size_t pud_module::count_axioms() const {
    return axioms_.size();
}

size_t pud_module::count_nodes() const {
    return children_.size();
}

pud_node_id pud_module::store_inference(pud_inference inference) {
    DEBUG_ASSERT(inference.parent < children_.size());
    const pud_node_id node = children_.size();
    std::vector<pud_node_id>& siblings = children_.at(inference.parent);
    DEBUG_ASSERT(siblings.empty() || get_inference(siblings.front()).call_site == inference.call_site);
    siblings.push_back(node);
    inferences_.push_back(std::move(inference));
    children_.emplace_back();
    refuted_.push_back(false);
    return node;
}

void pud_module::mark_refuted(pud_node_id node) {
    refuted_.at(node) = true;
}

const rule& pud_module::get_axiom(pud_node_id axiom) const {
    return axioms_.at(axiom);
}

const pud_inference& pud_module::get_inference(pud_node_id inference) const {
    DEBUG_ASSERT(inference >= axioms_.size());
    return inferences_.at(inference - axioms_.size());
}

std::optional<pud_node_id> pud_module::get_parent(pud_node_id node) const {
    if (node < axioms_.size())
        return std::nullopt;
    return get_inference(node).parent;
}

const std::vector<pud_node_id>& pud_module::get_children(pud_node_id node) const {
    return children_.at(node);
}

bool pud_module::check_leaf(pud_node_id node) const {
    return children_.at(node).empty();
}

bool pud_module::check_refuted(pud_node_id node) const {
    return refuted_.at(node);
}
