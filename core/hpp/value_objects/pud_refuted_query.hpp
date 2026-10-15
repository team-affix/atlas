#ifndef PUD_REFUTED_QUERY_HPP
#define PUD_REFUTED_QUERY_HPP

#include <compare>
#include "value_objects/pud_mhq_head_id.hpp"

struct pud_refuted_query {
    pud_mhq_head_id head_id;
    auto operator<=>(const pud_refuted_query&) const = default;
};

#endif
