#ifndef PUD_NODE_BASE_INTERVAL_HPP
#define PUD_NODE_BASE_INTERVAL_HPP

#include <unordered_map>
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

struct pud_node_base_interval {
    pud_node_base_interval();
    void store(const pud_rule_id* id, om_interval interval);
    const om_interval& get(const pud_rule_id* id) const;
private:
    using map_t = std::unordered_map<const pud_rule_id*, om_interval>;

    map_t by_id_;
};

#endif
