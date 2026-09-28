#include "infrastructure/pud_call_sites.hpp"
#include "debug_assert.hpp"

size_t pud_call_sites::get(const pud_node* node) const {
    return call_sites_.at(node);
}

void pud_call_sites::store(const pud_node* node, size_t call_site) {
    auto [_, success] = call_sites_.insert({node, call_site});
    DEBUG_ASSERT(success);
}
