#ifndef PUD_ROOT_NODES_HPP
#define PUD_ROOT_NODES_HPP

#include <unordered_set>
#include "value_objects/pud_node_id.hpp"

struct pud_root_nodes {
    bool check_root(pud_node_id id) const;
    void mark_root(pud_node_id id);
private:
    std::unordered_set<pud_node_id> roots_;
};

#endif
