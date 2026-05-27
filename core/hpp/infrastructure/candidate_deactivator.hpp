#ifndef CANDIDATE_DEACTIVATOR_HPP
#define CANDIDATE_DEACTIVATOR_HPP

#include "../interfaces/i_candidate_deactivator.hpp"
#include "../interfaces/i_unset_candidate_translation_map.hpp"
#include "../interfaces/i_deactivated_candidate_memory.hpp"

struct candidate_deactivator : i_candidate_deactivator {
    candidate_deactivator(
        i_unset_candidate_translation_map& unset_candidate_translation_map,
        i_deactivated_candidate_memory& deactivated_candidate_memory);
    void deactivate(const resolution_lineage*) override;
private:
    i_unset_candidate_translation_map& unset_candidate_translation_map;
    i_deactivated_candidate_memory& deactivated_candidate_memory;
};

#endif
