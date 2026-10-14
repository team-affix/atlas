#ifndef PUD_REPLAY_BIND_FACTORIES_HPP
#define PUD_REPLAY_BIND_FACTORIES_HPP

#include "infrastructure/pud_normalizer.hpp"
#include "infrastructure/pud_unifier.hpp"

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor>
struct pud_replay_bind_factories {
    pud_replay_bind_factories(IGlobalize& globalize, IMakeUbsFunctor& make_ubs_functor);

    using normalizer_t = global_normalizer<IGlobalize, IMakeUbsFunctor, BindMap>;
    using unifier_t = pud_unifier<IGlobalize, normalizer_t, BindMap, BindMap, BindMap>;

    normalizer_t make_normalizer(BindMap& bind_map);
    unifier_t make_unifier(BindMap& bind_map, normalizer_t& normalizer);

private:
    IGlobalize& globalize_;
    IMakeUbsFunctor& make_ubs_functor_;
};

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor>
pud_replay_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor>::pud_replay_bind_factories(
    IGlobalize& globalize,
    IMakeUbsFunctor& make_ubs_functor)
    : globalize_(globalize)
    , make_ubs_functor_(make_ubs_functor) {}

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor>
typename pud_replay_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor>::normalizer_t
pud_replay_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor>::make_normalizer(BindMap& bind_map) {
    return normalizer_t{globalize_, make_ubs_functor_, bind_map};
}

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor>
typename pud_replay_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor>::unifier_t
pud_replay_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor>::make_unifier(
    BindMap& bind_map,
    normalizer_t& normalizer) {
    return unifier_t{globalize_, normalizer, bind_map, bind_map, bind_map};
}

#endif
