#ifndef PUD_NODE_BASE_INTERVAL_HPP
#define PUD_NODE_BASE_INTERVAL_HPP

#include <unordered_map>
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

template<typename IAllocateRootInterval, typename IAllocateChildInterval>
struct pud_node_base_interval {
    pud_node_base_interval(IAllocateRootInterval& allocate_root_interval,
                           IAllocateChildInterval& allocate_child_interval);
    void bind_root(const pud_rule_id* id);
    void bind_child(const pud_rule_id* child, const pud_rule_id* parent);
    om_interval get(const pud_rule_id* id) const;
private:
    using map_t = std::unordered_map<const pud_rule_id*, om_interval>;

    IAllocateRootInterval& allocate_root_interval_;
    IAllocateChildInterval& allocate_child_interval_;
    map_t by_id_;
};

template<typename IARI, typename IACI>
pud_node_base_interval<IARI, IACI>::pud_node_base_interval(
        IARI& allocate_root_interval,
        IACI& allocate_child_interval)
    : allocate_root_interval_(allocate_root_interval)
    , allocate_child_interval_(allocate_child_interval)
    , by_id_() {}

template<typename IARI, typename IACI>
void pud_node_base_interval<IARI, IACI>::bind_root(const pud_rule_id* id) {
    DEBUG_ASSERT(!by_id_.contains(id));
    by_id_.emplace(id, allocate_root_interval_.allocate_root());
}

template<typename IARI, typename IACI>
void pud_node_base_interval<IARI, IACI>::bind_child(const pud_rule_id* child,
                                                   const pud_rule_id* parent) {
    DEBUG_ASSERT(!by_id_.contains(child));
    const om_interval parent_interval = by_id_.at(parent);
    by_id_.emplace(child, allocate_child_interval_.allocate_child_of(parent_interval));
}

template<typename IARI, typename IACI>
om_interval pud_node_base_interval<IARI, IACI>::get(const pud_rule_id* id) const {
    return by_id_.at(id);
}

#endif
