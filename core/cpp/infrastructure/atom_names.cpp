#include "infrastructure/atom_names.hpp"

atom_names::atom_names() {
    names.insert({k_nil_atom_id, "nil"});
    names.insert({k_cons_atom_id, "cons"});
}

bool atom_names::is_named(uint32_t id) const {
    return names.contains(id);
}

const std::string& atom_names::name(uint32_t id) const {
    return names.at(id);
}

void atom_names::set_name(uint32_t id, const std::string& name) {
    names.insert({id, name});
}
