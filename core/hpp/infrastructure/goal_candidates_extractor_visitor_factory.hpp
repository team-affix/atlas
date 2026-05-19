#ifndef GOAL_CANDIDATES_EXTRACTOR_VISITOR_FACTORY_HPP
#define GOAL_CANDIDATES_EXTRACTOR_VISITOR_FACTORY_HPP

#include <unordered_set>
#include "../interfaces/i_factory.hpp"
#include "../interfaces/i_goal_candidates_extractor_visitor.hpp"

struct goal_candidates_extractor_visitor_factory : i_factory<i_goal_candidates_extractor_visitor, std::unordered_set<const rule*>&> {
    virtual ~goal_candidates_extractor_visitor_factory() = default;
    std::unique_ptr<i_goal_candidates_extractor_visitor> make(std::unordered_set<const rule*>& extracted_candidates) const override;
};

#endif
