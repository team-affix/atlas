#include "infrastructure/pud_children.hpp"
#include "debug_assert.hpp"

const std::vector<pud_node_id> pud_children::empty_{};

const std::vector<pud_node_id>& pud_children::get(pud_node_id id) const {
    const auto it = children_links_.find(id);
    if (it == children_links_.end())
        return empty_;
    return it->second;
}

void pud_children::store(pud_node_id id, std::vector<pud_node_id> children) {
    auto [_, inserted] = children_links_.insert({id, std::move(children)});
    DEBUG_ASSERT(inserted);
}
