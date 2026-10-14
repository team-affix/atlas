#ifndef PUD_DESCENDER_BIND_FACTORIES_HPP
#define PUD_DESCENDER_BIND_FACTORIES_HPP

#include <cstdint>
#include <vector>
#include "infrastructure/normalizer.hpp"
#include "infrastructure/pud_normalizer.hpp"
#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/pud_whnf.hpp"
#include "infrastructure/representative_change_logger.hpp"

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor, typename IMakeVar, typename IMakeFunctor>
struct pud_descender_bind_factories {
    pud_descender_bind_factories(
        IGlobalize& globalize,
        IMakeUbsFunctor& make_ubs_functor,
        IMakeVar& make_var,
        IMakeFunctor& make_functor);

    using changed_rep_logger_t = representative_change_logger<BindMap, BindMap, std::vector<uint32_t>>;
    using normalizer_t = global_normalizer<IGlobalize, IMakeUbsFunctor, BindMap>;
    using unifier_t = pud_unifier<IGlobalize, normalizer_t, BindMap, changed_rep_logger_t, changed_rep_logger_t>;
    using whnf_t = pud_whnf<IGlobalize, BindMap>;
    using export_normalizer_t = normalizer<IGlobalize, IMakeFunctor, IMakeVar, whnf_t>;

    changed_rep_logger_t make_changed_rep_logger(BindMap& bind_map, std::vector<uint32_t>& changed_reps);
    normalizer_t make_normalizer(BindMap& bind_map);
    unifier_t make_unifier(
        BindMap& bind_map,
        normalizer_t& normalizer,
        changed_rep_logger_t& changed_rep_logger);
    whnf_t make_whnf(BindMap& bind_map);
    export_normalizer_t make_export_normalizer(BindMap& bind_map, whnf_t& whnf);

private:
    IGlobalize& globalize_;
    IMakeUbsFunctor& make_ubs_functor_;
    IMakeVar& make_var_;
    IMakeFunctor& make_functor_;
};

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor, typename IMakeVar, typename IMakeFunctor>
pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::pud_descender_bind_factories(
    IGlobalize& globalize,
    IMakeUbsFunctor& make_ubs_functor,
    IMakeVar& make_var,
    IMakeFunctor& make_functor)
    : globalize_(globalize)
    , make_ubs_functor_(make_ubs_functor)
    , make_var_(make_var)
    , make_functor_(make_functor) {}

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor, typename IMakeVar, typename IMakeFunctor>
typename pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::changed_rep_logger_t
pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::make_changed_rep_logger(
    BindMap& bind_map,
    std::vector<uint32_t>& changed_reps) {
    return changed_rep_logger_t{bind_map, bind_map, changed_reps};
}

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor, typename IMakeVar, typename IMakeFunctor>
typename pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::normalizer_t
pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::make_normalizer(
    BindMap& bind_map) {
    return normalizer_t{globalize_, make_ubs_functor_, bind_map};
}

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor, typename IMakeVar, typename IMakeFunctor>
typename pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::unifier_t
pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::make_unifier(
    BindMap& bind_map,
    normalizer_t& normalizer,
    changed_rep_logger_t& changed_rep_logger) {
    return unifier_t{globalize_, normalizer, bind_map, changed_rep_logger, changed_rep_logger};
}

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor, typename IMakeVar, typename IMakeFunctor>
typename pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::whnf_t
pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::make_whnf(
    BindMap& bind_map) {
    return whnf_t{globalize_, bind_map};
}

template<typename BindMap, typename IGlobalize, typename IMakeUbsFunctor, typename IMakeVar, typename IMakeFunctor>
typename pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::export_normalizer_t
pud_descender_bind_factories<BindMap, IGlobalize, IMakeUbsFunctor, IMakeVar, IMakeFunctor>::make_export_normalizer(
    BindMap& bind_map,
    whnf_t& whnf) {
    return export_normalizer_t{globalize_, make_functor_, make_var_, whnf};
}

#endif
