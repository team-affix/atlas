#ifndef PUD_FRONTIER_CHANGE_COLLECTOR_HPP
#define PUD_FRONTIER_CHANGE_COLLECTOR_HPP

#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_node_id.hpp"

// Records offspring and refuted nodes from one expand or open step.
template<typename ICheckNodeRefuted>
struct pud_frontier_change_collector {
    pud_frontier_change_collector(ICheckNodeRefuted& check_node_refuted);
    void add_produced(pud_node_id node);
    void add_refuted(pud_node_id node);
    pud_frontier_change finish();
private:
    ICheckNodeRefuted& check_node_refuted_;
    pud_frontier_change change_;
};

template<typename ICNR>
pud_frontier_change_collector<ICNR>::pud_frontier_change_collector(ICNR& check_node_refuted)
    : check_node_refuted_(check_node_refuted)
    , change_() {}

template<typename ICNR>
void pud_frontier_change_collector<ICNR>::add_produced(pud_node_id node) {
    if (check_node_refuted_.check_refuted(node))
        return;
    change_.offspring.push_back(node);
}

template<typename ICNR>
void pud_frontier_change_collector<ICNR>::add_refuted(pud_node_id node) {
    change_.refuted.push_back(node);
}

template<typename ICNR>
pud_frontier_change pud_frontier_change_collector<ICNR>::finish() {
    pud_frontier_change finished = change_;
    change_ = pud_frontier_change{.offspring = {}, .refuted = {}};
    return finished;
}

#endif
