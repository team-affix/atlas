#ifndef PUD_NODE_DEPTH_COUNTER_HPP
#define PUD_NODE_DEPTH_COUNTER_HPP

#include <cstddef>
#include <optional>
#include "value_objects/pud_global_node_id.hpp"

template<typename IGetParent>
struct pud_node_depth_counter {
    pud_node_depth_counter(IGetParent& get_parent);
    size_t count(pud_global_node_id node) const;
private:
    IGetParent& get_parent_;
};

template<typename IGetParent>
pud_node_depth_counter<IGetParent>::pud_node_depth_counter(IGetParent& get_parent)
    : get_parent_(get_parent) {}

template<typename IGetParent>
size_t pud_node_depth_counter<IGetParent>::count(pud_global_node_id node) const {
    size_t depth = 0;
    std::optional<pud_global_node_id> parent = get_parent_.get_parent(node);
    while (parent.has_value()) {
        ++depth;
        parent = get_parent_.get_parent(parent.value());
    }
    return depth;
}

#endif
