#ifndef I_GOAL_CANDIDATE_DEACTIVATOR_VISITOR_HPP
#define I_GOAL_CANDIDATE_DEACTIVATOR_VISITOR_HPP

#include "../interfaces/i_visitor.hpp"
#include "../value_objects/lineage.hpp"

struct i_goal_candidate_deactivator_visitor : i_visitor<const resolution_lineage*> {
    virtual ~i_goal_candidate_deactivator_visitor() = default;
};

#endif
