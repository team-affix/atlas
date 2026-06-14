#include "infrastructure/functor_names.hpp"

functor_names::functor_names() {
    names.insert({k_nil_functor_id, "nil"});
    names.insert({k_cons_functor_id, "cons"});
}

bool functor_names::is_named(uint32_t id) const {
    return names.contains(id);
}

const std::string& functor_names::name(uint32_t id) const {
    return names.at(id);
}

void functor_names::set_name(uint32_t id, const std::string& name) {
    names.insert({id, name});
}
