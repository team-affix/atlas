#ifndef PUD_REFUTED_NODES_HPP
#define PUD_REFUTED_NODES_HPP

#include <unordered_set>
#include "value_objects/pud_node_id.hpp"

struct pud_refuted_nodes {
    bool check_refuted(pud_node_id id) const;
    void set_refuted(pud_node_id id);
private:
    std::unordered_set<pud_node_id> refuted_;
};

#endif
