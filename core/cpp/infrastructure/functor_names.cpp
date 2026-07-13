#include "infrastructure/functor_names.hpp"

functor_names::functor_names() {
    names_.insert({k_nil_functor_id, "nil"});
    names_.insert({k_cons_functor_id, "cons"});
}

bool functor_names::is_named(uint32_t id) const {
    return names_.contains(id);
}

const std::string& functor_names::name(uint32_t id) const {
    return names_.at(id);
}

void functor_names::set_name(uint32_t id, const std::string& name) {
    names_.insert({id, name});
}
