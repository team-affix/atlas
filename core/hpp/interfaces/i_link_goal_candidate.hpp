#ifndef I_LINK_GOAL_CANDIDATE_HPP
#define I_LINK_GOAL_CANDIDATE_HPP

#include "../value_objects/lineage.hpp"

struct i_link_goal_candidate {
    virtual ~i_link_goal_candidate() = default;
    virtual void link_goal_candidate(const goal_lineage*, rule_id) = 0;
};

#endif
