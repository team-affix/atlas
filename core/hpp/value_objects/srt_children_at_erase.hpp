#ifndef SRT_CHILDREN_AT_ERASE_HPP
#define SRT_CHILDREN_AT_ERASE_HPP

#include <compare>

template<typename NodeId>
struct srt_children_at_erase {
    NodeId parent;
    NodeId child;
    auto operator<=>(const srt_children_at_erase&) const = default;
};

#endif
