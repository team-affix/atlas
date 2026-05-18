#ifndef I_GET_GOAL_CANDIDATES_SIZE_HPP
#define I_GET_GOAL_CANDIDATES_SIZE_HPP

#include "../value_objects/lineage.hpp"

struct i_get_goal_candidates_size {
    virtual ~i_get_goal_candidates_size() = default;
    virtual size_t size(const goal_lineage*) = 0;
};

#endif
