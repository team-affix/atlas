#ifndef I_LINK_GOAL_CANDIDATE_HPP
#define I_LINK_GOAL_CANDIDATE_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/rule.hpp"

struct i_link_goal_candidate {
    virtual ~i_link_goal_candidate() = default;
    virtual void link_goal_candidate(const goal_lineage*, const rule*) = 0;
};

#endif
