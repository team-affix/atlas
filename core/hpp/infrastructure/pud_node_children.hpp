#ifndef PUD_NODE_CHILDREN_HPP
#define PUD_NODE_CHILDREN_HPP

#include <optional>
#include <set>
#include <unordered_map>
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

struct pud_node_children {
    pud_node_children();
    void store(const pud_rule_id* id, std::set<const pud_rule_id*> children);
    std::optional<std::set<const pud_rule_id*>> get(const pud_rule_id* id) const;
private:
    using map_t = std::unordered_map<const pud_rule_id*, std::set<const pud_rule_id*>>;

    map_t by_id_;
};

#endif
