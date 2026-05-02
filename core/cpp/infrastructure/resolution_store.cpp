#include "../../hpp/infrastructure/resolution_store.hpp"

void resolution_store::insert(const resolution_lineage* rl) {
    resolutions.insert(rl);
}

void resolution_store::clear() {
    resolutions.clear();
}

lemma resolution_store::derive_lemma() {
    return lemma{resolutions};
}
