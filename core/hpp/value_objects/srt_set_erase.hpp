#ifndef SRT_SET_ERASE_HPP
#define SRT_SET_ERASE_HPP

#include <compare>
#include "value_objects/srt_set_target.hpp"

template<typename NodeId>
struct srt_set_erase {
    srt_set_target target;
    NodeId node;
    auto operator<=>(const srt_set_erase&) const = default;
};

#endif
