#include "infrastructure/var_names.hpp"

var_names::var_names() {}

bool var_names::is_named(uint32_t index) const {
    return names_.contains(index);
}

const std::string& var_names::name(uint32_t index) const {
    return names_.at(index);
}

void var_names::set_name(uint32_t index, const std::string& name) {
    names_.insert({index, name});
}
