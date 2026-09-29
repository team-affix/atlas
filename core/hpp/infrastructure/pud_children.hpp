#ifndef PUD_CHILDREN_HPP
#define PUD_CHILDREN_HPP

#include <unordered_map>
#include <vector>
#include "value_objects/pud_node.hpp"

struct pud_children {
    const std::vector<const pud_node*>& get(const pud_node* node) const;
    void store(const pud_node* node, const std::vector<const pud_node*>& children);
private:
    std::unordered_map<const pud_node*, std::vector<const pud_node*>> children_links_;
};

#endif
