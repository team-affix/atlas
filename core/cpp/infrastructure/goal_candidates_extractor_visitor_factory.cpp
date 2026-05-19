#include "../../hpp/infrastructure/goal_candidates_extractor_visitor_factory.hpp"
#include "../../hpp/infrastructure/goal_candidates_extractor_visitor.hpp"

std::unique_ptr<i_goal_candidates_extractor_visitor> goal_candidates_extractor_visitor_factory::make(std::unordered_set<const rule*>& extracted_candidates) const {
    return std::make_unique<goal_candidates_extractor_visitor>(extracted_candidates);
}
