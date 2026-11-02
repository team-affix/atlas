#include "value_objects/moduled_expr_hash.hpp"
#include <functional>

size_t moduled_expr_hash::operator()(const moduled_expr& term) const noexcept {
    const size_t skeleton_hash = std::hash<const expr*>{}(term.skeleton);
    const size_t framed_hash = skeleton_hash ^ (std::hash<uint32_t>{}(term.frame_offset) + 0x9e3779b97f4a7c15ull
        + (skeleton_hash << 6) + (skeleton_hash >> 2));
    return framed_hash ^ (std::hash<pud_module_id>{}(term.module) + 0x9e3779b97f4a7c15ull + (framed_hash << 6)
        + (framed_hash >> 2));
}
