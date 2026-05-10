#include "../../hpp/infrastructure/resolution_translation_map_store.hpp"

void resolution_translation_map_store::insert(const resolution_lineage* rl, std::unique_ptr<i_translation_map> tm) {
    maps.insert({rl, std::move(tm)});
}

bool resolution_translation_map_store::contains(const resolution_lineage* rl) const {
    return maps.contains(rl);
}

std::unique_ptr<i_translation_map>& resolution_translation_map_store::at(const resolution_lineage* rl) {
    return maps.at(rl);
}

const std::unique_ptr<i_translation_map>& resolution_translation_map_store::at(const resolution_lineage* rl) const {
    return maps.at(rl);
}

void resolution_translation_map_store::erase(const resolution_lineage* rl) {
    maps.erase(rl);
}

void resolution_translation_map_store::clear() {
    maps.clear();
}
