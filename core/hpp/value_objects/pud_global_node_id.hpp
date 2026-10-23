#ifndef PUD_GLOBAL_NODE_ID_HPP
#define PUD_GLOBAL_NODE_ID_HPP

#include <compare>
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"

// A node of any module: node ids are numbered within their module, so the same id names a different
// node in every module.
struct pud_global_node_id {
    pud_module_id module;
    pud_node_id node;
    auto operator<=>(const pud_global_node_id&) const = default;
};

#endif
