#ifndef PUD_LVC_DESCENT_HPP
#define PUD_LVC_DESCENT_HPP

#include <cstdint>

struct pud_lvc_descent {
    uint32_t lvc;
    auto operator<=>(const pud_lvc_descent&) const = default;
};

#endif
