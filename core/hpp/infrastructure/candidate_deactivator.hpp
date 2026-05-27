#ifndef CANDIDATE_DEACTIVATOR_HPP
#define CANDIDATE_DEACTIVATOR_HPP

#include "../interfaces/i_candidate_deactivator.hpp"
#include "../interfaces/i_deactivate_candidate_translation_map.hpp"

struct candidate_deactivator : i_candidate_deactivator {
    candidate_deactivator(i_deactivate_candidate_translation_map& dctm);
    void deactivate(const resolution_lineage*) override;
private:
    i_deactivate_candidate_translation_map& dctm;
};

#endif
