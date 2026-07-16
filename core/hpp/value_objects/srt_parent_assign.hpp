#ifndef SRT_PARENT_ASSIGN_HPP
#define SRT_PARENT_ASSIGN_HPP

#include <compare>

template<typename NodeId>
struct srt_parent_assign {
    NodeId child;
    NodeId previous;
    auto operator<=>(const srt_parent_assign&) const = default;
};

#endif
