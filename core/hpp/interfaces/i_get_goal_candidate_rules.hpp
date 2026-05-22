#ifndef I_GET_GOAL_CANDIDATE_RULES_HPP
#define I_GET_GOAL_CANDIDATE_RULES_HPP

#include "../interfaces/i_rule_set.hpp"
#include "../value_objects/lineage.hpp"

struct i_get_goal_candidate_rules {
    virtual ~i_get_goal_candidate_rules() = default;
    virtual i_rule_set& get(const goal_lineage*) = 0;
    virtual const i_rule_set& get(const goal_lineage*) const = 0;
};

#endif
