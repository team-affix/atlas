#ifndef PUD_NODE_CHILDREN_HPP
#define PUD_NODE_CHILDREN_HPP

#include <set>
#include <unordered_map>
#include <vector>
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

struct pud_node_children {
    pud_node_children();
    void add_root(const pud_rule_id* id);
    void link_children(const pud_rule_id* parent,
                       const std::vector<const pud_rule_id*>& children);
    std::vector<const pud_rule_id*> ordered_children(const pud_rule_id* parent) const;
    const pud_rule_id* find_parent(const pud_rule_id* child) const;
    bool is_leaf(const pud_rule_id* id) const;
private:
    struct rule_id_less {
        bool operator()(const pud_rule_id* a, const pud_rule_id* b) const;
    };
    using child_set_t = std::set<const pud_rule_id*, rule_id_less>;
    using children_map_t = std::unordered_map<const pud_rule_id*, child_set_t>;
    using parent_map_t = std::unordered_map<const pud_rule_id*, const pud_rule_id*>;

    children_map_t children_;
    parent_map_t parents_;
};

#endif
