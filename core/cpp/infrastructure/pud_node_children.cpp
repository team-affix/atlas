#include "infrastructure/pud_node_children.hpp"

pud_node_children::pud_node_children()
    : by_id_() {}

void pud_node_children::store(const pud_rule_id* id,
                              std::set<const pud_rule_id*> children) {
    DEBUG_ASSERT(!by_id_.contains(id));
    DEBUG_ASSERT(!children.empty());
    by_id_.emplace(id, std::move(children));
}

std::optional<std::set<const pud_rule_id*>>
pud_node_children::get(const pud_rule_id* id) const {
    auto it = by_id_.find(id);
    if (it == by_id_.end())
        return std::nullopt;
    return it->second;
}
