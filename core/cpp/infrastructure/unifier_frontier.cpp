#include "../../hpp/infrastructure/unifier_frontier.hpp"

void unifier_frontier::insert(const resolution_lineage* rl, std::unique_ptr<i_unifier> u) {
    maps.emplace(rl, std::move(u));
}

bool unifier_frontier::contains(const resolution_lineage* rl) const {
    return maps.contains(rl);
}

std::unique_ptr<i_unifier>& unifier_frontier::at(const resolution_lineage* rl) {
    return maps.at(rl);
}

const std::unique_ptr<i_unifier>& unifier_frontier::at(const resolution_lineage* rl) const {
    return maps.at(rl);
}

void unifier_frontier::erase(const resolution_lineage* rl) {
    maps.erase(rl);
}

void unifier_frontier::clear() {
    // Preserve the primary (nullptr key); erase only secondaries
    auto it = maps.begin();
    while (it != maps.end()) {
        if (it->first != nullptr)
            it = maps.erase(it);
        else
            ++it;
    }
}
