#ifndef PUD_LODESTAR_SIDE_HPP
#define PUD_LODESTAR_SIDE_HPP

#include <cstdint>

enum struct pud_lodestar_side : uint8_t {
    a = 0,
    b = 1,
    self = 2,
    caller = 3,
};

#endif
