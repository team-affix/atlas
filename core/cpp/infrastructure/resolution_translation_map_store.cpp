#include "../../hpp/infrastructure/resolution_translation_map_store.hpp"

void resolution_translation_map_store::insert(const resolution_lineage* rl, translation_map tm) {
    maps.insert({rl, std::move(tm)});
}

translation_map& resolution_translation_map_store::get(const resolution_lineage* rl) {
    return maps.at(rl);
}

void resolution_translation_map_store::erase(const resolution_lineage* rl) {
    maps.erase(rl);
}

void resolution_translation_map_store::clear() {
    maps.clear();
}
