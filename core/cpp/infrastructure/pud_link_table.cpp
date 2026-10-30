#include "infrastructure/pud_link_table.hpp"

#include "debug_assert.hpp"

pud_link_table::pud_link_table()
    : rows_() {}

void pud_link_table::insert_link(pud_module_id module, uint32_t local_functor_id, uint32_t global_functor_id) {
    module_links& row = rows_[module];
    const auto local_link = row.global_by_local.emplace(local_functor_id, global_functor_id);
    const auto global_link = row.local_by_global.emplace(global_functor_id, local_functor_id);
    DEBUG_ASSERT(local_link.second);
    DEBUG_ASSERT(global_link.second);
}

bool pud_link_table::contains_module(pud_module_id module) const {
    return rows_.contains(module);
}

bool pud_link_table::contains_global_functor_id(pud_module_id module, uint32_t global_functor_id) const {
    const auto found = rows_.find(module);
    if (found == rows_.end())
        return false;
    return found->second.local_by_global.contains(global_functor_id);
}

uint32_t pud_link_table::get_global_functor_id_from_local_functor_id(
    pud_module_id module, uint32_t local_functor_id) const {
    return rows_.at(module).global_by_local.at(local_functor_id);
}

uint32_t pud_link_table::get_local_functor_id_from_global_functor_id(
    pud_module_id module, uint32_t global_functor_id) const {
    return rows_.at(module).local_by_global.at(global_functor_id);
}
