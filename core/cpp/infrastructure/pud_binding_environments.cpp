#include "infrastructure/pud_binding_environments.hpp"

void pud_binding_environments::record(const pud_node* node, map_t bindings) {
    envs_.insert_or_assign(node, std::move(bindings));
}

const pud_binding_environments::map_t&
pud_binding_environments::query(const pud_node* node) const {
    return envs_.at(node);
}
