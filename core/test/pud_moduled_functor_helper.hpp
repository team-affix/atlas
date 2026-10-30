#ifndef PUD_MODULED_FUNCTOR_HELPER_HPP
#define PUD_MODULED_FUNCTOR_HELPER_HPP

#include "value_objects/expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/ubs_class.hpp"

inline moduled_functor pud_test_moduled_functor(const expr* skeleton, uint32_t frame_offset) {
    return moduled_functor{std::get_if<expr::functor>(&skeleton->content), frame_offset, 0};
}

inline ubs_class pud_test_bound_class(uint32_t root, uint32_t rep, const expr* skeleton, uint32_t frame_offset) {
    return ubs_class{.root = root, .size = 1, .rep = rep, .value = pud_test_moduled_functor(skeleton, frame_offset)};
}

#endif
