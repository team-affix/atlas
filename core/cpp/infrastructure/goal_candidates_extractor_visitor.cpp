#include "../../hpp/infrastructure/goal_candidates_extractor_visitor.hpp"

goal_candidates_extractor_visitor::goal_candidates_extractor_visitor(std::unordered_set<const rule*>& extracted_candidates)
    : extracted_candidates(extracted_candidates) {}

void goal_candidates_extractor_visitor::visit(const rule* r) {
    extracted_candidates.insert(r);
}
