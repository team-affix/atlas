#ifndef PUD_SPECIALIZER_HPP
#define PUD_SPECIALIZER_HPP

#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_specialization.hpp"

template<typename IMakeVar, typename IUnify>
struct pud_specializer {
    pud_specializer(
        IMakeVar& make_var,
        IUnify& unify);
    bool specialize(uint32_t frame_offset, pud_specialization specialization);
private:
    IMakeVar& make_var_;
    IUnify& unify_;
};

template<typename IMakeVar, typename IUnify>
bool pud_specializer<IMakeVar, IUnify>::specialize(uint32_t frame_offset, pud_specialization specialization) {
    // frame the lhs and rhs with same frame offset
    framed_expr lhs{make_var_.make_var(specialization.global_var_idx), frame_offset};
    framed_expr rhs{specialization.value, frame_offset};
    // unify the lhs and rhs
    return unify_.unify(lhs, rhs);
}

#endif
