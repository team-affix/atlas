#ifndef CANDIDATE_ACTIVATOR_HPP
#define CANDIDATE_ACTIVATOR_HPP

#include "../interfaces/i_candidate_activator.hpp"

struct candidate_activator : i_candidate_activator {
    candidate_activator();
    void activate(const resolution_lineage*) override;
};

#endif
