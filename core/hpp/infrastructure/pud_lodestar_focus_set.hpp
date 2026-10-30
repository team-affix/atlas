#ifndef PUD_LODESTAR_FOCUS_SET_HPP
#define PUD_LODESTAR_FOCUS_SET_HPP

#include <unordered_set>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_global_node_id_hash.hpp"

struct pud_lodestar_focus_set {
    pud_lodestar_focus_set();
    void insert(pud_global_node_id node);
    bool contains(pud_global_node_id node) const;
private:
    std::unordered_set<pud_global_node_id, pud_global_node_id_hash> nodes_;
};

#endif
