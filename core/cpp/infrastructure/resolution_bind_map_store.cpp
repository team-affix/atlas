#include "../../hpp/infrastructure/resolution_bind_map_store.hpp"

void resolution_bind_map_store::insert(const resolution_lineage* rl, std::unique_ptr<i_unifier> u) {
    maps.emplace(rl, std::move(u));
}

bool resolution_bind_map_store::contains(const resolution_lineage* rl) const {
    return maps.contains(rl);
}

std::unique_ptr<i_unifier>& resolution_bind_map_store::at(const resolution_lineage* rl) {
    return maps.at(rl);
}

const std::unique_ptr<i_unifier>& resolution_bind_map_store::at(const resolution_lineage* rl) const {
    return maps.at(rl);
}

void resolution_bind_map_store::erase(const resolution_lineage* rl) {
    maps.erase(rl);
}

void resolution_bind_map_store::clear() {
    // Preserve the primary (nullptr key); erase only secondaries
    auto it = maps.begin();
    while (it != maps.end()) {
        if (it->first != nullptr)
            it = maps.erase(it);
        else
            ++it;
    }
}
