#ifndef PUD_GLOBAL_NODE_ID_HASH_HPP
#define PUD_GLOBAL_NODE_ID_HASH_HPP

#include <cstddef>
#include "value_objects/pud_global_node_id.hpp"

struct pud_global_node_id_hash {
    size_t operator()(const pud_global_node_id& id) const noexcept;
};

#endif
