#ifndef PUD_WITNESS_PAIR_HPP
#define PUD_WITNESS_PAIR_HPP

#include <compare>
#include "value_objects/pud_witness_search_context.hpp"

struct pud_witness_pair {
    pud_witness_search_context a;
    pud_witness_search_context b;
    auto operator<=>(const pud_witness_pair&) const = default;
};

#endif
