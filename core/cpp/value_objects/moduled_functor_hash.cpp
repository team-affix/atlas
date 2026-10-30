#include "value_objects/moduled_functor_hash.hpp"
#include <functional>

size_t moduled_functor_hash::operator()(const moduled_functor& functor) const noexcept {
    const size_t functor_hash = std::hash<const expr::functor*>{}(functor.functor);
    const size_t mixed = functor_hash ^ (std::hash<uint32_t>{}(functor.frame_offset) + 0x9e3779b97f4a7c15ull
        + (functor_hash << 6) + (functor_hash >> 2));
    return mixed ^ (std::hash<pud_module_id>{}(functor.module) + 0x9e3779b97f4a7c15ull + (mixed << 6) + (mixed >> 2));
}
