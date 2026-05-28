#include "infrastructure/resolution_memory.hpp"

void resolution_memory::record_resolution(const resolution_lineage* rl) {
    resolutions.insert(rl);
}

void resolution_memory::clear_recorded_resolutions() {
    resolutions.clear();
}

size_t resolution_memory::get_resolution_count() const {
    return resolutions.size();
}

lemma resolution_memory::derive_resolution_lemma() const {
    return lemma{resolutions};
}
