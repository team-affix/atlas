#ifndef PUD_WHNF_HPP
#define PUD_WHNF_HPP

#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

template<typename IGlobalize, typename IResolve>
struct pud_whnf {
    pud_whnf(IGlobalize& globalize, IResolve& resolve);
    framed_expr whnf(framed_expr fe);

private:
    IGlobalize& globalize_;
    IResolve& resolve_;
};

template<typename IGlobalize, typename IResolve>
pud_whnf<IGlobalize, IResolve>::pud_whnf(IGlobalize& globalize, IResolve& resolve)
    : globalize_(globalize)
    , resolve_(resolve) {}

template<typename IGlobalize, typename IResolve>
framed_expr pud_whnf<IGlobalize, IResolve>::whnf(framed_expr fe) {
    const expr::var* var = std::get_if<expr::var>(&fe.skeleton->content);
    if (!var)
        return fe;
    const uint32_t global_key = globalize_.globalize(fe.frame_offset, var->index);
    return framed_expr{resolve_.resolve(global_key), 0u};
}

#endif
