#ifndef PUD_REFUTED_NODES_HPP
#define PUD_REFUTED_NODES_HPP

#include <unordered_set>
#include "value_objects/pud_node.hpp"

struct pud_refuted_nodes {
    bool check_refuted(const pud_node* node) const;
    void set_refuted(const pud_node* node);
private:
    std::unordered_set<const pud_node*> refuted_;
};

#endif
