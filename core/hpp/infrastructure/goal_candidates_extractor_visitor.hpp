#ifndef GOAL_CANDIDATES_EXTRACTOR_VISITOR_HPP
#define GOAL_CANDIDATES_EXTRACTOR_VISITOR_HPP

#include <unordered_set>
#include "../interfaces/i_goal_candidates_extractor_visitor.hpp"

struct goal_candidates_extractor_visitor : i_goal_candidates_extractor_visitor {
    goal_candidates_extractor_visitor(std::unordered_set<const rule*>&);
    void visit(const rule*) override;
private:
    std::unordered_set<const rule*>& extracted_candidates;
};

#endif
