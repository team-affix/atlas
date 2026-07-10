#ifndef SRT_PARENT_ERASE_HPP
#define SRT_PARENT_ERASE_HPP

#include <compare>

template<typename NodeId>
struct srt_parent_erase {
    NodeId child;
    NodeId parent;
    auto operator<=>(const srt_parent_erase&) const = default;
};

#endif
