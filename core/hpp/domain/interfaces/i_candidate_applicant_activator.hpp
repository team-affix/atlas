#ifndef I_CANDIDATE_APPLICANT_ACTIVATOR_HPP
#define I_CANDIDATE_APPLICANT_ACTIVATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_candidate_applicant_activator {
    virtual ~i_candidate_applicant_activator() = default;
    virtual void activate(const resolution_lineage*) = 0;
};

#endif
