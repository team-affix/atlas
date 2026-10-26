#ifndef PUD_INFERENCE_HPP
#define PUD_INFERENCE_HPP

#include <compare>
#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_substitution.hpp"

struct pud_inference {
    pud_node_id parent;
    pud_body_goal_id call_site;
    std::vector<pud_substitution> substitutions;
    std::vector<const expr*> body_goals;
    uint32_t var_count;
    auto operator<=>(const pud_inference&) const = default;
};

#endif
