#ifndef I_CANDIDATES_FRONTIER_HPP
#define I_CANDIDATES_FRONTIER_HPP

#include "../value_objects/candidate_set.hpp"
#include "../value_objects/lineage.hpp"
#include "i_map.hpp"

struct i_candidates_frontier : i_map<const goal_lineage*, candidate_set> {
    virtual ~i_candidates_frontier() = default;
};

#endif
