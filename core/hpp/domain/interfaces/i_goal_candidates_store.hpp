#ifndef I_GOAL_CANDIDATES_STORE_HPP
#define I_GOAL_CANDIDATES_STORE_HPP

#include "../value_objects/candidate_set.hpp"
#include "i_goal_store.hpp"

struct i_goal_candidates_store : i_goal_store<const candidate_set&> {
    virtual ~i_goal_candidates_store() = default;
    virtual void eliminate(const resolution_lineage*) = 0;
};

#endif
