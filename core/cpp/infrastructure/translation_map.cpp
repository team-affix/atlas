#include <cstdint>
#include "../../hpp/infrastructure/translation_map.hpp"

void translation_map::insert(uint32_t k, uint32_t v) {
    entries.insert({k, v});
}

bool translation_map::contains(uint32_t k) const {
    return entries.contains(k);
}

uint32_t& translation_map::at(uint32_t k) {
    return entries.at(k);
}

const uint32_t& translation_map::at(uint32_t k) const {
    return entries.at(k);
}

void translation_map::erase(uint32_t k) {
    entries.erase(k);
}

void translation_map::clear() {
    entries.clear();
}
