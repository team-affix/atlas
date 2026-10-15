#ifndef PUD_REPLAY_DESCENT_HPP
#define PUD_REPLAY_DESCENT_HPP

#include <cstdint>
#include <optional>
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_specialization.hpp"
#include "value_objects/pud_ubs_state.hpp"

template<
    typename BindMap,
    typename Normalizer,
    typename Unifier,
    typename IMakeUbsVar,
    typename IMakeUbsFunctor,
    typename IMakeVar,
    typename IMakeNormalizer,
    typename IMakeUnifier,
    typename IGetNodeAddedSpecializations>
struct pud_replay_descent {
    pud_replay_descent(
        IMakeUbsVar& make_ubs_var,
        IMakeUbsFunctor& make_ubs_functor,
        IMakeVar& make_var,
        IMakeNormalizer& make_normalizer,
        IMakeUnifier& make_unifier,
        IGetNodeAddedSpecializations& get_node_added_specializations,
        const pud_ubs_state& caller_ubs_state,
        uint32_t frame_offset);
    std::optional<pud_replay_descent> descend(pud_node_id child) const;
    const pud_ubs_state& ubs_state() const;
private:
    IMakeUbsVar* make_ubs_var_;
    IMakeUbsFunctor* make_ubs_functor_;
    IMakeVar* make_var_;
    IMakeNormalizer* make_normalizer_;
    IMakeUnifier* make_unifier_;
    IGetNodeAddedSpecializations* get_node_added_specializations_;
    uint32_t frame_offset_;
    pud_ubs_state ubs_state_;
};

#define PUD_REPLAY_DESCENT_TEMPLATE                                                       \
    template<typename BindMap, typename Normalizer, typename Unifier, typename IMakeUbsVar, \
             typename IMakeUbsFunctor, typename IMakeVar, typename IMakeNormalizer,         \
             typename IMakeUnifier, typename IGetNodeAddedSpecializations>
#define PUD_REPLAY_DESCENT                                                                \
    pud_replay_descent<BindMap, Normalizer, Unifier, IMakeUbsVar, IMakeUbsFunctor,        \
        IMakeVar, IMakeNormalizer, IMakeUnifier, IGetNodeAddedSpecializations>

PUD_REPLAY_DESCENT_TEMPLATE
PUD_REPLAY_DESCENT::pud_replay_descent(
    IMakeUbsVar& make_ubs_var,
    IMakeUbsFunctor& make_ubs_functor,
    IMakeVar& make_var,
    IMakeNormalizer& make_normalizer,
    IMakeUnifier& make_unifier,
    IGetNodeAddedSpecializations& get_node_added_specializations,
    const pud_ubs_state& caller_ubs_state,
    uint32_t frame_offset)
    : make_ubs_var_(&make_ubs_var)
    , make_ubs_functor_(&make_ubs_functor)
    , make_var_(&make_var)
    , make_normalizer_(&make_normalizer)
    , make_unifier_(&make_unifier)
    , get_node_added_specializations_(&get_node_added_specializations)
    , frame_offset_(frame_offset)
    , ubs_state_(caller_ubs_state) {}

PUD_REPLAY_DESCENT_TEMPLATE
std::optional<PUD_REPLAY_DESCENT> PUD_REPLAY_DESCENT::descend(pud_node_id child) const {
    pud_transient_ubs_state transient_ubs_state{ubs_state_};
    BindMap bind_map = transient_ubs_state.make_bind_map(*make_ubs_var_, *make_ubs_functor_);
    Normalizer normalizer = make_normalizer_->make_normalizer(bind_map);
    Unifier unifier = make_unifier_->make_unifier(bind_map, normalizer);
    for (const pud_specialization& spec : get_node_added_specializations_->get(child)) {
        framed_expr lhs{make_var_->make_var(spec.var_idx), frame_offset_};
        framed_expr rhs{spec.value, frame_offset_};
        if (!unifier.unify(lhs, rhs))
            return std::nullopt;
    }
    PUD_REPLAY_DESCENT result = *this;
    result.ubs_state_ = transient_ubs_state.to_persistent_state();
    return result;
}

PUD_REPLAY_DESCENT_TEMPLATE
const pud_ubs_state& PUD_REPLAY_DESCENT::ubs_state() const {
    return ubs_state_;
}

#undef PUD_REPLAY_DESCENT
#undef PUD_REPLAY_DESCENT_TEMPLATE

#endif
