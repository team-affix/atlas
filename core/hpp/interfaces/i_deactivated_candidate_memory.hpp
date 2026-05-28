#ifndef I_DEACTIVATED_CANDIDATE_MEMORY_HPP
#define I_DEACTIVATED_CANDIDATE_MEMORY_HPP

#include "value_objects/lineage.hpp"

struct i_deactivated_candidate_memory {
    virtual ~i_deactivated_candidate_memory() = default;
    virtual void insert(const resolution_lineage*) = 0;
    virtual void clear() = 0;
    virtual bool contains(const resolution_lineage*) const = 0;
};

#endif
