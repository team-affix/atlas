#ifndef MHU_ARENA_EMPLACE_HPP
#define MHU_ARENA_EMPLACE_HPP

#include <compare>

struct mhu_arena_emplace {
    auto operator<=>(const mhu_arena_emplace&) const = default;
};

#endif
