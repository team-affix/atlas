#include "../../hpp/infrastructure/resolution_memory.hpp"

void resolution_memory::insert(const resolution_lineage* rl) {
    resolutions.insert(rl);
}

void resolution_memory::clear() {
    resolutions.clear();
}

lemma resolution_memory::derive_lemma() const {
    return lemma{resolutions};
}
