#ifndef PUD_NODE_DELTA_HPP
#define PUD_NODE_DELTA_HPP

#include <compare>
#include <cstdint>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_substitution.hpp"

// An inference: its parent resolved on its call site, and what it adds on top of that parent: its
// substitutions, its body goals and how many fresh variables they introduce.
struct pud_node_delta {
    pud_node_id parent;
    pud_body_goal_id call_site;
    std::vector<pud_substitution> substitutions;
    std::vector<const expr*> body_goals;
    uint32_t var_count;
    auto operator<=>(const pud_node_delta&) const = default;
};

#endif
