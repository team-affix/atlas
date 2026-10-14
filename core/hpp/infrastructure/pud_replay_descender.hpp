#ifndef PUD_REPLAY_DESCENDER_HPP
#define PUD_REPLAY_DESCENDER_HPP

#include <optional>
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_replay_descent.hpp"
#include "value_objects/pud_specialization.hpp"

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
struct pud_replay_descender {
    pud_replay_descender(
        IMakeUbsVar& make_ubs_var,
        IMakeUbsFunctor& make_ubs_functor,
        IMakeVar& make_var,
        IMakeNormalizer& make_normalizer,
        IMakeUnifier& make_unifier,
        IGetNodeAddedSpecializations& get_node_added_specializations);
    pud_replay_descent root_descent(const pud_ubs_state& caller_ubs_state, uint32_t frame_offset);
    std::optional<pud_replay_descent> descend(
        const pud_replay_descent& descent,
        pud_node_id parent,
        pud_node_id child);

private:
    IMakeUbsVar& make_ubs_var_;
    IMakeUbsFunctor& make_ubs_functor_;
    IMakeVar& make_var_;
    IMakeNormalizer& make_normalizer_;
    IMakeUnifier& make_unifier_;
    IGetNodeAddedSpecializations& get_node_added_specializations_;
};

#define PUD_REPLAY_DESCENDER_TEMPLATE                                                     \
    template<typename BindMap, typename Normalizer, typename Unifier, typename IMakeUbsVar, \
             typename IMakeUbsFunctor, typename IMakeVar, typename IMakeNormalizer,         \
             typename IMakeUnifier, typename IGetNodeAddedSpecializations>
#define PUD_REPLAY_DESCENDER                                                              \
    pud_replay_descender<BindMap, Normalizer, Unifier, IMakeUbsVar, IMakeUbsFunctor,      \
        IMakeVar, IMakeNormalizer, IMakeUnifier, IGetNodeAddedSpecializations>

PUD_REPLAY_DESCENDER_TEMPLATE
PUD_REPLAY_DESCENDER::pud_replay_descender(
    IMakeUbsVar& make_ubs_var,
    IMakeUbsFunctor& make_ubs_functor,
    IMakeVar& make_var,
    IMakeNormalizer& make_normalizer,
    IMakeUnifier& make_unifier,
    IGetNodeAddedSpecializations& get_node_added_specializations)
    : make_ubs_var_(make_ubs_var)
    , make_ubs_functor_(make_ubs_functor)
    , make_var_(make_var)
    , make_normalizer_(make_normalizer)
    , make_unifier_(make_unifier)
    , get_node_added_specializations_(get_node_added_specializations) {}

PUD_REPLAY_DESCENDER_TEMPLATE
pud_replay_descent PUD_REPLAY_DESCENDER::root_descent(
    const pud_ubs_state& caller_ubs_state,
    uint32_t frame_offset) {
    return pud_replay_descent{
        .frame_offset = frame_offset,
        .ubs_state    = caller_ubs_state,
    };
}

PUD_REPLAY_DESCENDER_TEMPLATE
std::optional<pud_replay_descent> PUD_REPLAY_DESCENDER::descend(
    const pud_replay_descent& descent,
    pud_node_id parent,
    pud_node_id child) {
    pud_transient_ubs_state transient_ubs_state{descent.ubs_state};
    BindMap bind_map = transient_ubs_state.make_bind_map(make_ubs_var_, make_ubs_functor_);
    Normalizer normalizer = make_normalizer_.make_normalizer(bind_map);
    Unifier unifier = make_unifier_.make_unifier(bind_map, normalizer);

    const uint32_t frame_offset = descent.frame_offset;
    for (const pud_specialization& spec : get_node_added_specializations_.get(child)) {
        framed_expr lhs{make_var_.make_var(spec.var_idx), frame_offset};
        framed_expr rhs{spec.value, frame_offset};
        if (!unifier.unify(lhs, rhs))
            return std::nullopt;
    }

    return pud_replay_descent{
        .frame_offset = frame_offset,
        .ubs_state    = transient_ubs_state.to_persistent_state(),
    };
}

#undef PUD_REPLAY_DESCENDER
#undef PUD_REPLAY_DESCENDER_TEMPLATE

#endif
