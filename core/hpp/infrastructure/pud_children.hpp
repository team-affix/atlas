#ifndef PUD_CHILDREN_HPP
#define PUD_CHILDREN_HPP

#include <unordered_map>
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_node.hpp"

struct pud_children {
    using children_map = std::unordered_map<const pud_rule_id*, const pud_node*>;
    children_map get(const pud_node* node) const;
    void store(const pud_node* node, children_map children);
private:
    std::unordered_map<const pud_node*, children_map> children_links_;
};

#endif
