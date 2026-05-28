#include "infrastructure/deactivated_candidate_memory.hpp"

deactivated_candidate_memory::deactivated_candidate_memory() {}

void deactivated_candidate_memory::insert(const resolution_lineage* rl) {
    deactivated_candidates.insert(rl);
}

void deactivated_candidate_memory::clear() {
    deactivated_candidates.clear();
}

bool deactivated_candidate_memory::contains(const resolution_lineage* rl) const {
    return deactivated_candidates.contains(rl);
}
