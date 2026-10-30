#include "infrastructure/pud_symbol_table.hpp"

pud_symbol_table::pud_symbol_table()
    : global_by_name_()
    , name_by_global_() {
    intern_functor_name("[]");
    intern_functor_name("cons");
}

uint32_t pud_symbol_table::intern_functor_name(const std::string& name) {
    const auto found = global_by_name_.find(name);
    if (found != global_by_name_.end())
        return found->second;
    const uint32_t global_functor_id = static_cast<uint32_t>(global_by_name_.size());
    global_by_name_.emplace(name, global_functor_id);
    name_by_global_.emplace(global_functor_id, name);
    return global_functor_id;
}

uint32_t pud_symbol_table::get_global_functor_id_by_name(const std::string& name) const {
    return global_by_name_.at(name);
}

bool pud_symbol_table::contains_named_functor_id(uint32_t global_functor_id) const {
    return name_by_global_.contains(global_functor_id);
}

const std::string& pud_symbol_table::get_name_by_global_functor_id(uint32_t global_functor_id) const {
    return name_by_global_.at(global_functor_id);
}
