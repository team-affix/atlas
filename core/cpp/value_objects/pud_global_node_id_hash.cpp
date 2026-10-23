#include "value_objects/pud_global_node_id_hash.hpp"
#include <functional>

size_t pud_global_node_id_hash::operator()(const pud_global_node_id& id) const noexcept {
    const size_t module_hash = std::hash<pud_module_id>{}(id.module);
    return module_hash ^ (std::hash<pud_node_id>{}(id.node) + 0x9e3779b97f4a7c15ull + (module_hash << 6)
        + (module_hash >> 2));
}
