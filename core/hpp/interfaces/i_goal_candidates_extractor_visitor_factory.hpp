#ifndef I_GOAL_CANDIDATES_EXTRACTOR_VISITOR_FACTORY_HPP
#define I_GOAL_CANDIDATES_EXTRACTOR_VISITOR_FACTORY_HPP

#include <unordered_set>
#include "../interfaces/i_factory.hpp"
#include "../interfaces/i_goal_candidates_extractor_visitor.hpp"

struct i_goal_candidates_extractor_visitor_factory : i_factory<i_goal_candidates_extractor_visitor, std::unordered_set<const rule*>&> {
    virtual ~i_goal_candidates_extractor_visitor_factory() = default;
};

#endif
