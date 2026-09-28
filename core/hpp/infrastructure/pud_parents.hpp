#ifndef PUD_PARENTS_HPP
#define PUD_PARENTS_HPP

#include <unordered_map>
#include "value_objects/pud_node.hpp"

struct pud_parents {
    const pud_node* get(const pud_node* node) const;
    void store(const pud_node* node, const pud_node* parent);
private:
    std::unordered_map<const pud_node*, const pud_node*> parent_links_;
};

#endif
