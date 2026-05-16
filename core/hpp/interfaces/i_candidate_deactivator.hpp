#ifndef I_CANDIDATE_DEACTIVATOR_HPP
#define I_CANDIDATE_DEACTIVATOR_HPP

#include "../value_objects/lineage.hpp"

struct i_candidate_deactivator {
    virtual ~i_candidate_deactivator() = default;
    virtual bool deactivate(const resolution_lineage*) = 0;
};

#endif
