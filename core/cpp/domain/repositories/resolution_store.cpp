#include "../../../../hpp/domain/entities/resolution_store/resolution_store.hpp"

resolution_store::resolution_store() :
    resolutions() {
}

void resolution_store::insert(const resolution_lineage* rl) {
    resolutions.insert(rl);
}

const std::unordered_set<const resolution_lineage*>& resolution_store::get() const {
    return resolutions;
}

void resolution_store::clear() {
    resolutions.clear();
}
