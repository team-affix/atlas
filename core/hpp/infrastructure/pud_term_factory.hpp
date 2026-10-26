#ifndef PUD_TERM_FACTORY_HPP
#define PUD_TERM_FACTORY_HPP

#include <cstdint>
#include <unordered_map>
#include "infrastructure/pud_cycle_detector.hpp"
#include "infrastructure/pud_exporter.hpp"
#include "infrastructure/pud_normalizer.hpp"
#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Makes the per-environment term machinery: a bind map over a state, and the cycle detector, unifier,
// normalizer and exporter that work on that bind map. Consumers depend on one maker each, so each can
// be replaced by a mock.
template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar>
struct pud_term_factory {
    using cycle_detector_t = pud_cycle_detector<IGlobalize, ubs_bind_map, IMakeFunctor>;
    using unifier_t = pud_unifier<IGlobalize, ubs_bind_map, ubs_bind_map, ubs_bind_map, cycle_detector_t, IMakeFunctor>;
    using normalizer_t = pud_normalizer<IGlobalize, IMakeFunctor, IMakeVar, ubs_bind_map>;
    using exporter_t = pud_exporter<IGlobalize, IMakeFunctor, IMakeVar, ubs_bind_map>;

    pud_term_factory(IGlobalize& globalize, IMakeFunctor& make_functor, IMakeVar& make_var);
    ubs_bind_map make_bind_map(const pud_ubs_state& state) const;
    cycle_detector_t make_cycle_detector(ubs_bind_map& bind_map) const;
    unifier_t make_unifier(ubs_bind_map& bind_map, cycle_detector_t& cycle_detector) const;
    normalizer_t make_normalizer(ubs_bind_map& bind_map) const;
    exporter_t make_exporter(
        ubs_bind_map& bind_map,
        uint32_t caller_lvc,
        std::unordered_map<uint32_t, uint32_t>& translations) const;
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
};

#define PUD_TERM_FACTORY_TEMPLATE template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar>
#define PUD_TERM_FACTORY pud_term_factory<IGlobalize, IMakeFunctor, IMakeVar>

PUD_TERM_FACTORY_TEMPLATE
PUD_TERM_FACTORY::pud_term_factory(IGlobalize& globalize, IMakeFunctor& make_functor, IMakeVar& make_var)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var) {}

PUD_TERM_FACTORY_TEMPLATE
ubs_bind_map PUD_TERM_FACTORY::make_bind_map(const pud_ubs_state& state) const {
    return ubs_bind_map{state};
}

PUD_TERM_FACTORY_TEMPLATE
typename PUD_TERM_FACTORY::cycle_detector_t PUD_TERM_FACTORY::make_cycle_detector(ubs_bind_map& bind_map) const {
    return cycle_detector_t{globalize_, bind_map, make_functor_};
}

PUD_TERM_FACTORY_TEMPLATE
typename PUD_TERM_FACTORY::unifier_t PUD_TERM_FACTORY::make_unifier(
    ubs_bind_map& bind_map, cycle_detector_t& cycle_detector) const {
    return unifier_t{globalize_, bind_map, bind_map, bind_map, cycle_detector, make_functor_};
}

PUD_TERM_FACTORY_TEMPLATE
typename PUD_TERM_FACTORY::normalizer_t PUD_TERM_FACTORY::make_normalizer(ubs_bind_map& bind_map) const {
    return normalizer_t{globalize_, make_functor_, make_var_, bind_map};
}

PUD_TERM_FACTORY_TEMPLATE
typename PUD_TERM_FACTORY::exporter_t PUD_TERM_FACTORY::make_exporter(
    ubs_bind_map& bind_map,
    uint32_t caller_lvc,
    std::unordered_map<uint32_t, uint32_t>& translations) const {
    return exporter_t{globalize_, make_functor_, make_var_, bind_map, caller_lvc, translations};
}

#undef PUD_TERM_FACTORY
#undef PUD_TERM_FACTORY_TEMPLATE

#endif
