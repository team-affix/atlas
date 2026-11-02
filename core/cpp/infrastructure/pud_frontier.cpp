#include "infrastructure/pud_frontier.hpp"

pud_frontier::pud_frontier()
    : live_() {}

void pud_frontier::apply_register(const pud_frontier_change& change) {
    live_.insert(change.offspring.begin(), change.offspring.end());
    erase_refuted(change.refuted);
}

void pud_frontier::apply_unfold(pud_global_node_id parent, const pud_frontier_change& change) {
    live_.erase(parent);
    live_.insert(change.offspring.begin(), change.offspring.end());
    erase_refuted(change.refuted);
}

void pud_frontier::erase_refuted(const std::vector<pud_global_node_id>& refuted) {
    for (const pud_global_node_id node : refuted)
        live_.erase(node);
}

void pud_frontier::erase(pud_global_node_id node) {
    live_.erase(node);
}

const std::set<pud_global_node_id>& pud_frontier::live() const {
    return live_;
}
