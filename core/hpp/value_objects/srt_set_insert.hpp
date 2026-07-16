#ifndef SRT_SET_INSERT_HPP
#define SRT_SET_INSERT_HPP

#include <compare>
#include "value_objects/srt_set_target.hpp"

template<typename NodeId>
struct srt_set_insert {
    srt_set_target target;
    NodeId node;
    auto operator<=>(const srt_set_insert&) const = default;
};

#endif
