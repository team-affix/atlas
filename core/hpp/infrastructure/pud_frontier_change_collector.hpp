#ifndef PUD_FRONTIER_CHANGE_COLLECTOR_HPP
#define PUD_FRONTIER_CHANGE_COLLECTOR_HPP

#include <vector>
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_node_id.hpp"

// New nodes from one step that are still live, plus every leaf refuted during the step.
template<typename ICheckNodeRefuted>
struct pud_frontier_change_collector {
    pud_frontier_change_collector(ICheckNodeRefuted& check_node_refuted);
    pud_frontier_change collect(const std::vector<pud_node_id>& produced, const std::vector<pud_node_id>& refuted) const;
private:
    ICheckNodeRefuted& check_node_refuted_;
};

template<typename ICNR>
pud_frontier_change_collector<ICNR>::pud_frontier_change_collector(ICNR& check_node_refuted)
    : check_node_refuted_(check_node_refuted) {}

template<typename ICNR>
pud_frontier_change pud_frontier_change_collector<ICNR>::collect(
    const std::vector<pud_node_id>& produced, const std::vector<pud_node_id>& refuted) const {
    pud_frontier_change change{.offspring = {}, .refuted = refuted};
    for (pud_node_id node : produced) {
        if (check_node_refuted_.check_refuted(node))
            continue;
        change.offspring.push_back(node);
    }
    return change;
}

#endif
