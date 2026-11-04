#ifndef PUD_LEAF_EVALUATION_HPP
#define PUD_LEAF_EVALUATION_HPP

#include <compare>
#include <map>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_eager_candidate.hpp"
#include "value_objects/pud_global_node_id.hpp"

// A visited leaf's goals and the candidates each has now. A refuted leaf has a goal without any; its goals after
// that one were not queried.
struct pud_leaf_evaluation {
    pud_global_node_id leaf;
    bool refuted;
    std::map<pud_body_goal_id, std::vector<pud_eager_candidate>> goals;
    auto operator<=>(const pud_leaf_evaluation&) const = default;
};

#endif
