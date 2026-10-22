#include "infrastructure/pud_node_added_substitutions.hpp"
#include "debug_assert.hpp"

const std::vector<pud_substitution>& pud_node_added_substitutions::get(pud_node_id id) const {
    return substitutions_.at(id);
}

void pud_node_added_substitutions::store(pud_node_id id, std::vector<pud_substitution> substitutions) {
    auto [_, inserted] = substitutions_.insert({id, std::move(substitutions)});
    DEBUG_ASSERT(inserted);
}
