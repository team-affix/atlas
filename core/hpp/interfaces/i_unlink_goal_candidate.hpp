#ifndef I_UNLINK_GOAL_CANDIDATE_HPP
#define I_UNLINK_GOAL_CANDIDATE_HPP

#include "../value_objects/lineage.hpp"

struct i_unlink_goal_candidate {
    virtual ~i_unlink_goal_candidate() = default;
    virtual void unlink_goal_candidate(const goal_lineage*, rule_id) = 0;
};

#endif
