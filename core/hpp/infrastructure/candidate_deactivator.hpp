#ifndef CANDIDATE_DEACTIVATOR_HPP
#define CANDIDATE_DEACTIVATOR_HPP

#include "../interfaces/i_candidate_deactivator.hpp"

struct candidate_deactivator : i_candidate_deactivator {
    candidate_deactivator();
    void deactivate(const resolution_lineage*) override;
};

#endif
