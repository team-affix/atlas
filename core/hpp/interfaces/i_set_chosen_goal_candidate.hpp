#ifndef I_SET_CHOSEN_GOAL_CANDIDATE_HPP
#define I_SET_CHOSEN_GOAL_CANDIDATE_HPP

#include "value_objects/lineage.hpp"

struct i_set_chosen_goal_candidate {
    virtual ~i_set_chosen_goal_candidate() = default;
    virtual void set(const goal_lineage*, rule_id) = 0;
};

#endif
