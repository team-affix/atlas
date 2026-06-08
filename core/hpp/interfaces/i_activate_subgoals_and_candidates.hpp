#ifndef I_ACTIVATE_SUBGOALS_AND_CANDIDATES_HPP
#define I_ACTIVATE_SUBGOALS_AND_CANDIDATES_HPP

#include "value_objects/lineage.hpp"

struct i_activate_subgoals_and_candidates {
    virtual ~i_activate_subgoals_and_candidates() = default;
    virtual bool activate_subgoals_and_candidates(const resolution_lineage*) = 0;
};

#endif
