#ifndef I_UNLINK_GOAL_CANDIDATE_HPP
#define I_UNLINK_GOAL_CANDIDATE_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/rule.hpp"

struct i_unlink_goal_candidate {
    virtual ~i_unlink_goal_candidate() = default;
    virtual void unlink_goal_candidate(const goal_lineage*, const rule*) = 0;
};

#endif
