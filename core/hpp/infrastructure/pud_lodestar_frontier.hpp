#ifndef PUD_LODESTAR_FRONTIER_HPP
#define PUD_LODESTAR_FRONTIER_HPP

#include <set>
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"

struct pud_lodestar_frontier {
    pud_lodestar_frontier();
    void apply_register(const pud_frontier_change& change);
    void apply_unfold(pud_global_node_id parent, const pud_frontier_change& change);
    void erase_refuted(const std::vector<pud_global_node_id>& refuted);
    const std::set<pud_global_node_id>& live() const;
private:
    std::set<pud_global_node_id> live_;
};

#endif
