#ifndef I_GOAL_CANDIDATES_EXTRACTOR_VISITOR_HPP
#define I_GOAL_CANDIDATES_EXTRACTOR_VISITOR_HPP

#include "../interfaces/i_visitor.hpp"
#include "../value_objects/rule.hpp"

struct i_goal_candidates_extractor_visitor : i_visitor<const rule*> {
    virtual ~i_goal_candidates_extractor_visitor() = default;
};

#endif
