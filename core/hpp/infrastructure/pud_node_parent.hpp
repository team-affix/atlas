#ifndef PUD_NODE_PARENT_HPP
#define PUD_NODE_PARENT_HPP

#include <unordered_map>
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

struct pud_node_parent {
    pud_node_parent();
    void store(const pud_rule_id* child, const pud_rule_id* parent);
    const pud_rule_id* get(const pud_rule_id* child) const;
private:
    using map_t = std::unordered_map<const pud_rule_id*, const pud_rule_id*>;

    map_t by_id_;
};

#endif
