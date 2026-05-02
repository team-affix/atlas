#ifndef I_GOAL_CANDIDATES_STORE_HPP
#define I_GOAL_CANDIDATES_STORE_HPP

#include "../value_objects/candidate_set.hpp"
#include "../value_objects/lineage.hpp"

struct i_goal_candidates_store {
    virtual ~i_goal_candidates_store() = default;
    virtual void insert(const goal_lineage*, const candidate_set&) = 0;
    virtual void erase(const goal_lineage*) = 0;
    virtual void eliminate(const goal_lineage*, size_t) = 0;
    virtual void clear() = 0;
    virtual const candidate_set& at(const goal_lineage*) const = 0;
    virtual size_t size() const = 0;
};

#endif
