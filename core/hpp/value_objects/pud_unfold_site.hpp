#ifndef PUD_UNFOLD_SITE_HPP
#define PUD_UNFOLD_SITE_HPP

#include <compare>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_candidate_search_context.hpp"

struct pud_unfold_site {
    const expr* body_goal;
    std::vector<pud_candidate_search_context*> live;
    auto operator<=>(const pud_unfold_site&) const = default;
};

#endif
