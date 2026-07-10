#ifndef SRT_PARENT_INSERT_HPP
#define SRT_PARENT_INSERT_HPP

#include <compare>

template<typename NodeId>
struct srt_parent_insert {
    NodeId child;
    NodeId parent;
    auto operator<=>(const srt_parent_insert&) const = default;
};

#endif
