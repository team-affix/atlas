#ifndef SRT_CHILDREN_INSERT_HPP
#define SRT_CHILDREN_INSERT_HPP

#include <compare>
#include <set>

template<typename NodeId>
struct srt_children_insert {
    NodeId parent;
    std::set<NodeId> children;
    auto operator<=>(const srt_children_insert&) const = default;
};

#endif
