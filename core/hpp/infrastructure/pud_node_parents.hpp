#ifndef PUD_NODE_PARENTS_HPP
#define PUD_NODE_PARENTS_HPP

#include <unordered_map>
#include "value_objects/pud_node_id.hpp"

struct pud_node_parents {
    pud_node_id get(pud_node_id id) const;
    void store(pud_node_id id, pud_node_id parent);
private:
    std::unordered_map<pud_node_id, pud_node_id> parents_;
};

#endif
