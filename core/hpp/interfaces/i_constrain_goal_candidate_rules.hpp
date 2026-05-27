#ifndef I_CONSTRAIN_GOAL_CANDIDATE_RULES_HPP
#define I_CONSTRAIN_GOAL_CANDIDATE_RULES_HPP

#include "../value_objects/lineage.hpp"

struct i_constrain_goal_candidate_rules {
    virtual ~i_constrain_goal_candidate_rules() = default;
    virtual void constrain_goal_candidate_rules(const resolution_lineage*) = 0;
};

#endif
