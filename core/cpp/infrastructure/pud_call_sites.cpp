#include "infrastructure/pud_call_sites.hpp"
#include "debug_assert.hpp"

bool pud_call_sites::check_call_site(pud_node_id id) const {
    return call_sites_.contains(id);
}

size_t pud_call_sites::get(pud_node_id id) const {
    return call_sites_.at(id);
}

void pud_call_sites::store(pud_node_id id, size_t call_site) {
    auto [_, success] = call_sites_.insert({id, call_site});
    DEBUG_ASSERT(success);
}
