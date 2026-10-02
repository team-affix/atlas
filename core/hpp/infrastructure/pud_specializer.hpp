#ifndef PUD_SPECIALIZER_HPP
#define PUD_SPECIALIZER_HPP

#include "infrastructure/coroutine.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_specialization.hpp"

template<typename IMakeVar, typename IUnify>
struct pud_specializer {
    pud_specializer(
        IMakeVar& make_var,
        IUnify& unify);
    coroutine<uint32_t, bool> specialize(uint32_t frame_offset, pud_specialization specialization);
private:
    IMakeVar& make_var_;
    IUnify& unify_;
};

template<typename IMakeVar, typename IUnify>
pud_specializer<IMakeVar, IUnify>::pud_specializer(IMakeVar& make_var, IUnify& unify)
    : make_var_(make_var)
    , unify_(unify) {}

template<typename IMakeVar, typename IUnify>
coroutine<uint32_t, bool> pud_specializer<IMakeVar, IUnify>::specialize(uint32_t frame_offset, pud_specialization specialization) {
    // frame the lhs and rhs with same frame offset
    framed_expr lhs{make_var_.make_var(specialization.var_idx), frame_offset};
    framed_expr rhs{specialization.value, frame_offset};
    // unify the lhs and rhs
    auto sm = unify_.unify(lhs, rhs);
    while (auto touched_rep = sm.next()) {
        if (*touched_rep < frame_offset)
            // only yield those vars which
            // became specialized from the caller env
            co_yield *touched_rep;
    }
    co_return sm.result();
}

#endif
