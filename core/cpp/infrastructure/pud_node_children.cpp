#include "infrastructure/pud_node_children.hpp"

bool pud_node_children::rule_id_less::operator()(const pud_rule_id* a,
                                                 const pud_rule_id* b) const {
    return *a < *b;
}

pud_node_children::pud_node_children()
    : children_()
    , parents_() {}

void pud_node_children::add_root(const pud_rule_id* id) {
    DEBUG_ASSERT(!children_.contains(id));
    DEBUG_ASSERT(!parents_.contains(id));
    children_.emplace(id, child_set_t{});
}

void pud_node_children::link_children(const pud_rule_id* parent,
                                      const std::vector<const pud_rule_id*>& children) {
    DEBUG_ASSERT(is_leaf(parent));
    DEBUG_ASSERT(!children.empty());
    child_set_t child_set;
    for (const pud_rule_id* child : children) {
        DEBUG_ASSERT(!parents_.contains(child));
        DEBUG_ASSERT(child != parent);
        child_set.insert(child);
    }
    child_set_t& stored = children_[parent];
    for (const pud_rule_id* child : child_set) {
        stored.insert(child);
        parents_.emplace(child, parent);
        children_.emplace(child, child_set_t{});
    }
}

std::vector<const pud_rule_id*>
pud_node_children::ordered_children(const pud_rule_id* parent) const {
    auto it = children_.find(parent);
    if (it == children_.end())
        return {};
    return std::vector<const pud_rule_id*>(it->second.begin(), it->second.end());
}

const pud_rule_id* pud_node_children::find_parent(const pud_rule_id* child) const {
    auto it = parents_.find(child);
    if (it == parents_.end())
        return nullptr;
    return it->second;
}

bool pud_node_children::is_leaf(const pud_rule_id* id) const {
    auto it = children_.find(id);
    if (it == children_.end())
        return false;
    return it->second.empty();
}
