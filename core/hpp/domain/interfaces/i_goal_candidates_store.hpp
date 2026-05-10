#ifndef I_GOAL_CANDIDATES_STORE_HPP
#define I_GOAL_CANDIDATES_STORE_HPP

#include "../value_objects/candidate_set.hpp"
#include "../value_objects/lineage.hpp"
#include "i_map.hpp"

struct i_goal_candidates_store : i_map<const goal_lineage*, candidate_set> {
    virtual ~i_goal_candidates_store() = default;
    virtual void eliminate(const resolution_lineage*) = 0;
};

#endif
