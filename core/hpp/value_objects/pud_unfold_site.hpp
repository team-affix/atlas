#ifndef PUD_UNFOLD_SITE_HPP
#define PUD_UNFOLD_SITE_HPP

#include <compare>
#include <vector>
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"

struct pud_unfold_site {
    pud_query* query;
    std::vector<const pud_rule_id*> callees;
    auto operator<=>(const pud_unfold_site&) const = default;
};

#endif
