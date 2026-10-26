#include "value_objects/framed_functor_hash.hpp"
#include <functional>

size_t framed_functor_hash::operator()(const framed_functor& framed) const noexcept {
    const size_t functor_hash = std::hash<const expr::functor*>{}(framed.functor);
    const size_t mixed = functor_hash;
    return mixed ^ (std::hash<uint32_t>{}(framed.frame_offset) + 0x9e3779b97f4a7c15ull + (mixed << 6) + (mixed >> 2));
}
