#ifndef I_CANDIDATE_STORE_HPP
#define I_CANDIDATE_STORE_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/candidate_set.hpp"

struct i_candidate_store {
    virtual ~i_candidate_store() = default;
    virtual void insert(const goal_lineage*, const candidate_set&) = 0;
    virtual void erase(const goal_lineage*) = 0;
    virtual void eliminate(const goal_lineage*, size_t) = 0;
    virtual void clear() = 0;
    virtual const candidate_set& at(const goal_lineage*) = 0;
};

#endif
