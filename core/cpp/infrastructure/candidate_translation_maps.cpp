#include "../../hpp/infrastructure/candidate_translation_maps.hpp"

translation_map& candidate_translation_maps::get(const resolution_lineage* rl) {
    return maps_[rl];
}

void candidate_translation_maps::set(const resolution_lineage* rl, translation_map tm) {
    maps_[rl] = std::move(tm);
}

void candidate_translation_maps::unset(const resolution_lineage* rl) {
    maps_.erase(rl);
}
