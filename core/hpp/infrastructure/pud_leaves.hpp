#ifndef PUD_LEAVES_HPP
#define PUD_LEAVES_HPP

#include <unordered_set>
#include "value_objects/pud_node.hpp"

struct pud_leaves {
    bool check_leaf(const pud_node* node) const;
    void set_leaf(const pud_node* node);
    void unset_leaf(const pud_node* node);
private:
    std::unordered_set<const pud_node*> leaves_;
};

#endif
