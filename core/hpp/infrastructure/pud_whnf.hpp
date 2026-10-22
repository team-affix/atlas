#ifndef PUD_WHNF_HPP
#define PUD_WHNF_HPP

#include <optional>
#include <utility>
#include <variant>
#include "infrastructure/pud_normalizer.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

template<typename IGlobalize, typename IMakeVar, typename IMakeFunctor>
struct pud_whnf {
    pud_whnf(IGlobalize& globalize, IMakeVar& make_var, IMakeFunctor& make_functor, pud_ubs_state ubs_state);
    const expr* whnf(const expr* skeleton) const;
    const expr* materialize(const expr* skeleton) const;
private:
    IGlobalize* globalize_;
    IMakeVar* make_var_;
    IMakeFunctor* make_functor_;
    pud_ubs_state ubs_state_;
};

template<typename IG, typename IMV, typename IMF>
pud_whnf<IG, IMV, IMF>::pud_whnf(IG& globalize, IMV& make_var, IMF& make_functor, pud_ubs_state ubs_state)
    : globalize_(&globalize)
    , make_var_(&make_var)
    , make_functor_(&make_functor)
    , ubs_state_(std::move(ubs_state)) {}

template<typename IG, typename IMV, typename IMF>
const expr* pud_whnf<IG, IMV, IMF>::whnf(const expr* skeleton) const {
    pud_transient_ubs_state transient_ubs_state{ubs_state_};
    auto bind_map = transient_ubs_state.make_bind_map();
    while (const expr::var* var = std::get_if<expr::var>(&skeleton->content)) {
        const std::optional<framed_expr> value = bind_map.find_value(var->index);
        if (!value.has_value())
            return make_var_->make_var(bind_map.find_rep(var->index));
        skeleton = value->skeleton;
    }
    return skeleton;
}

template<typename IG, typename IMV, typename IMF>
const expr* pud_whnf<IG, IMV, IMF>::materialize(const expr* skeleton) const {
    pud_transient_ubs_state transient_ubs_state{ubs_state_};
    auto bind_map = transient_ubs_state.make_bind_map();
    global_normalizer<IG, IMF, IMV, decltype(bind_map), decltype(bind_map)> normalizer{
        *globalize_, *make_functor_, *make_var_, bind_map, bind_map};
    return normalizer.normalize(framed_expr{skeleton, 0});
}

#endif
