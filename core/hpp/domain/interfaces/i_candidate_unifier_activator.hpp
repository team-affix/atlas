#ifndef I_CANDIDATE_UNIFIER_ACTIVATOR_HPP
#define I_CANDIDATE_UNIFIER_ACTIVATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_candidate_unifier_activator {
    virtual ~i_candidate_unifier_activator() = default;
    virtual void activate(const resolution_lineage*) = 0;
};

#endif
