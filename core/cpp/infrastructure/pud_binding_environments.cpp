#include "infrastructure/pud_binding_environments.hpp"

void pud_binding_environments::record(pud_node_id id, map_t bindings) {
    envs_.insert_or_assign(id, std::move(bindings));
}

const pud_binding_environments::map_t&
pud_binding_environments::query(pud_node_id id) const {
    return envs_.at(id);
}
