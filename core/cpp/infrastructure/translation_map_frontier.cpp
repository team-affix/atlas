#include "../../hpp/infrastructure/translation_map_frontier.hpp"

void translation_map_frontier::insert(const resolution_lineage* rl, std::unique_ptr<i_translation_map> tm) {
    maps.insert({rl, std::move(tm)});
}

bool translation_map_frontier::contains(const resolution_lineage* rl) const {
    return maps.contains(rl);
}

std::unique_ptr<i_translation_map>& translation_map_frontier::at(const resolution_lineage* rl) {
    return maps.at(rl);
}

const std::unique_ptr<i_translation_map>& translation_map_frontier::at(const resolution_lineage* rl) const {
    return maps.at(rl);
}

void translation_map_frontier::erase(const resolution_lineage* rl) {
    maps.erase(rl);
}

void translation_map_frontier::clear() {
    maps.clear();
}
