#include "infrastructure/pud_data.hpp"
#include <utility>
#include "debug_assert.hpp"

pud_data::pud_data()
    : axioms_()
    , inferences_()
    , children_() {}

pud_node_id pud_data::store_axiom(const rule& axiom) {
    const pud_node_id node = children_.size();
    axioms_.emplace(node, axiom);
    children_.emplace_back();
    return node;
}

pud_node_id pud_data::store_inference(pud_node_delta delta) {
    DEBUG_ASSERT(delta.parent < children_.size());
    const pud_node_id node = children_.size();
    std::vector<pud_node_id>& siblings = children_.at(delta.parent);
    DEBUG_ASSERT(siblings.empty() || inferences_.at(siblings.front()).call_site == delta.call_site);
    siblings.push_back(node);
    inferences_.emplace(node, std::move(delta));
    children_.emplace_back();
    return node;
}

const rule& pud_data::get_axiom(pud_node_id axiom) const {
    return axioms_.at(axiom);
}

const pud_node_delta& pud_data::get_inference(pud_node_id inference) const {
    return inferences_.at(inference);
}

std::optional<pud_node_id> pud_data::get_parent(pud_node_id node) const {
    const auto inference = inferences_.find(node);
    if (inference == inferences_.end())
        return std::nullopt;
    return inference->second.parent;
}

const std::vector<pud_node_id>& pud_data::get_children(pud_node_id node) const {
    return children_.at(node);
}

bool pud_data::check_leaf(pud_node_id node) const {
    return children_.at(node).empty();
}
