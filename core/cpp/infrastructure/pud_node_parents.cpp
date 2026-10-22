#include "infrastructure/pud_node_parents.hpp"
#include "debug_assert.hpp"

pud_node_id pud_node_parents::get(pud_node_id id) const {
    return parents_.at(id);
}

void pud_node_parents::store(pud_node_id id, pud_node_id parent) {
    auto [_, inserted] = parents_.insert({id, parent});
    DEBUG_ASSERT(inserted);
}
