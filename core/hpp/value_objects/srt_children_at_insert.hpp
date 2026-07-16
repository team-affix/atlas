#ifndef SRT_CHILDREN_AT_INSERT_HPP
#define SRT_CHILDREN_AT_INSERT_HPP

#include <compare>

template<typename NodeId>
struct srt_children_at_insert {
    NodeId parent;
    NodeId child;
    auto operator<=>(const srt_children_at_insert&) const = default;
};

#endif
