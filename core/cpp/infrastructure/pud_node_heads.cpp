#include "infrastructure/pud_node_heads.hpp"
#include "debug_assert.hpp"

const expr* pud_node_heads::get(pud_node_id id) const {
    return heads_.at(id);
}

void pud_node_heads::store(pud_node_id id, const expr* head) {
    auto [_, inserted] = heads_.insert({id, head});
    DEBUG_ASSERT(inserted);
}
