#ifndef PUD_NODE_REFUTED_HPP
#define PUD_NODE_REFUTED_HPP

#include <unordered_map>
#include "value_objects/pud_node.hpp"

struct pud_node_refuted {
    void store(const pud_node* node, bool refuted);
    bool get(const pud_node* node) const;    
private:
    std::unordered_map<const pud_node*, bool> node_refuted_;
};

#endif
