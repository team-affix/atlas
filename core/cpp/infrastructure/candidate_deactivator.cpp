#include "../../hpp/infrastructure/candidate_deactivator.hpp"

candidate_deactivator::candidate_deactivator(i_deactivate_candidate_translation_map& dctm)
    : dctm(dctm) {}

void candidate_deactivator::deactivate(const resolution_lineage* rl) {
    dctm.deactivate(rl);
}
