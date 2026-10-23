#include "value_objects/framed_expr_hash.hpp"
#include <functional>

size_t framed_expr_hash::operator()(const framed_expr& framed) const noexcept {
    const size_t skeleton_hash = std::hash<const expr*>{}(framed.skeleton);
    return skeleton_hash ^ (std::hash<uint32_t>{}(framed.frame_offset) + 0x9e3779b97f4a7c15ull + (skeleton_hash << 6)
        + (skeleton_hash >> 2));
}
