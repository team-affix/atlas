#ifndef SRT_CHILDREN_ERASE_HPP
#define SRT_CHILDREN_ERASE_HPP

#include <compare>
#include <set>

template<typename NodeId>
struct srt_children_erase {
    NodeId parent;
    std::set<NodeId> children;
    auto operator<=>(const srt_children_erase&) const = default;
};

#endif
