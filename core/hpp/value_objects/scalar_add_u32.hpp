#ifndef SCALAR_ADD_U32_HPP
#define SCALAR_ADD_U32_HPP

#include <compare>
#include <cstdint>

struct scalar_add_u32 {
    uint32_t amount;
    auto operator<=>(const scalar_add_u32&) const = default;
};

#endif
