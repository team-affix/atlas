#ifndef PUD_ROOT_NODES_HPP
#define PUD_ROOT_NODES_HPP

#include <unordered_set>
#include <vector>
#include "value_objects/pud_node_id.hpp"

struct pud_root_nodes {
    bool check_root(pud_node_id id) const;
    void mark_root(pud_node_id id);
    const std::vector<pud_node_id>& get() const;
private:
    std::unordered_set<pud_node_id> root_set_;
    std::vector<pud_node_id> roots_;
};

#endif
