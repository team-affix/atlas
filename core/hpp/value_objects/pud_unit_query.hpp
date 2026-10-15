#ifndef PUD_UNIT_QUERY_HPP
#define PUD_UNIT_QUERY_HPP

#include <compare>
#include "value_objects/pud_mhq_head_id.hpp"

struct pud_unit_query {
    pud_mhq_head_id head_id;
    auto operator<=>(const pud_unit_query&) const = default;
};

#endif
