#ifndef I_CANDIDATE_INITIALIZER_HPP
#define I_CANDIDATE_INITIALIZER_HPP

#include "../value_objects/lineage.hpp"

struct i_candidate_initializer {
    virtual ~i_candidate_initializer() = default;
    virtual void seed_expansion(const goal_lineage*) = 0;
    virtual void initialize(const resolution_lineage*) = 0;
};

#endif
