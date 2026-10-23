#ifndef PUD_FRONTIER_CHANGE_COLLECTOR_HPP
#define PUD_FRONTIER_CHANGE_COLLECTOR_HPP

#include <vector>
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_node_id.hpp"

// Sorts the leaves an operation produced into opened and proven, leaving out those it then refuted.
template<typename ICheckNodeRefuted, typename IGetNodeGoals>
struct pud_frontier_change_collector {
    pud_frontier_change_collector(ICheckNodeRefuted& check_node_refuted, IGetNodeGoals& get_node_goals);
    pud_frontier_change collect(const std::vector<pud_node_id>& produced, const std::vector<pud_node_id>& refuted) const;
private:
    ICheckNodeRefuted& check_node_refuted_;
    IGetNodeGoals& get_node_goals_;
};

template<typename ICNR, typename IGNG>
pud_frontier_change_collector<ICNR, IGNG>::pud_frontier_change_collector(ICNR& check_node_refuted, IGNG& get_node_goals)
    : check_node_refuted_(check_node_refuted)
    , get_node_goals_(get_node_goals) {}

template<typename ICNR, typename IGNG>
pud_frontier_change pud_frontier_change_collector<ICNR, IGNG>::collect(
    const std::vector<pud_node_id>& produced, const std::vector<pud_node_id>& refuted) const {
    pud_frontier_change change{.opened = {}, .proven = {}, .refuted = refuted};
    for (pud_node_id leaf : produced) {
        if (check_node_refuted_.check_refuted(leaf))
            continue;
        const bool has_open_goals = !get_node_goals_.get(leaf).goals.empty();
        (has_open_goals ? change.opened : change.proven).push_back(leaf);
    }
    return change;
}

#endif
