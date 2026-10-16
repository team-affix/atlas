#ifndef PUD_LEAF_NODES_HPP
#define PUD_LEAF_NODES_HPP

#include <unordered_set>
#include "value_objects/pud_node_id.hpp"

struct pud_leaf_nodes {
    bool check_leaf(pud_node_id id) const;
    void set_leaf(pud_node_id id);
    void unset_leaf(pud_node_id id);
private:
    std::unordered_set<pud_node_id> leaves_;
};

#endif
