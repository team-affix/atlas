#ifndef PUD_DERIVATION_ID_HPP
#define PUD_DERIVATION_ID_HPP

#include <compare>
#include <cstdint>

struct pud_derivation_id {
    uint32_t value;
    auto operator<=>(const pud_derivation_id&) const = default;
};

#endif
