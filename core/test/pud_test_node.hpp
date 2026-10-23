#ifndef PUD_TEST_NODE_HPP
#define PUD_TEST_NODE_HPP

#include <ostream>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/pud_node_id.hpp"

// The module unit tests draw their nodes in, when only one module is involved.
inline constexpr pud_module_id k_test_module = 7;

inline pud_global_node_id pud_test_node(pud_node_id node) {
    return pud_global_node_id{.module = k_test_module, .node = node};
}

inline void PrintTo(const pud_global_node_id& id, std::ostream* out) {
    *out << "(module " << id.module << ", node " << id.node << ")";
}

#endif
