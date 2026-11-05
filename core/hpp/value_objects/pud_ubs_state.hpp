#ifndef PUD_UBS_STATE_HPP
#define PUD_UBS_STATE_HPP

#include <cstdint>
#include <functional>
#include <immer/map.hpp>
#include <immer/memory_policy.hpp>
#include "value_objects/ubs_class.hpp"

// Bits of a variable index consumed per level of the binding maps: a node holds up to 2^bits entries or children.
// Every live leaf keeps its own version, so a narrow node keeps each edit's path copy small.
inline constexpr immer::detail::hamts::bits_t k_ubs_map_bits = 3;

template<typename T>
using ubs_map = immer::map<
    uint32_t, T, std::hash<uint32_t>, std::equal_to<uint32_t>, immer::default_memory_policy, k_ubs_map_bits>;

struct pud_ubs_state {
    ubs_map<uint32_t> parents;
    ubs_map<ubs_class> classes;
    bool operator==(const pud_ubs_state&) const = default;
};

#endif
