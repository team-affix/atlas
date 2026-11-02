#include "infrastructure/pud_symbol_table.hpp"

#include "infrastructure/functor_names.hpp"
#include "debug_assert.hpp"

pud_symbol_table::pud_symbol_table()
    : global_by_name_()
    , name_by_global_() {
    const uint32_t nil = intern_functor_name("[]");
    const uint32_t cons = intern_functor_name("cons");
    DEBUG_ASSERT(nil == k_nil_functor_id);
    DEBUG_ASSERT(cons == k_cons_functor_id);
}

uint32_t pud_symbol_table::intern_functor_name(const std::string& name) {
    const auto found = global_by_name_.find(name);
    if (found != global_by_name_.end())
        return found->second;
    const uint32_t global_functor_id = static_cast<uint32_t>(name_by_global_.size());
    global_by_name_.emplace(name, global_functor_id);
    name_by_global_.push_back(name);
    return global_functor_id;
}

const std::string& pud_symbol_table::get_name_by_global_functor_id(uint32_t global_functor_id) const {
    return name_by_global_.at(global_functor_id);
}
