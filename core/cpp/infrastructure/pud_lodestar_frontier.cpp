#include "infrastructure/pud_lodestar_frontier.hpp"

pud_lodestar_frontier::pud_lodestar_frontier()
    : live_() {}

void pud_lodestar_frontier::apply_register(const pud_frontier_change& change) {
    live_.insert(change.offspring.begin(), change.offspring.end());
    erase_refuted(change.refuted);
}

void pud_lodestar_frontier::apply_unfold(pud_global_node_id parent, const pud_frontier_change& change) {
    live_.erase(parent);
    live_.insert(change.offspring.begin(), change.offspring.end());
    erase_refuted(change.refuted);
}

void pud_lodestar_frontier::erase_refuted(const std::vector<pud_global_node_id>& refuted) {
    for (const pud_global_node_id node : refuted)
        live_.erase(node);
}

const std::set<pud_global_node_id>& pud_lodestar_frontier::live() const {
    return live_;
}
