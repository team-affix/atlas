#ifndef PUD_UNIFIER_FACTORY_HPP
#define PUD_UNIFIER_FACTORY_HPP

#include "infrastructure/pud_cycle_detector_factory.hpp"
#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/ubs_bind_map.hpp"

template<typename IGlobalize, typename IMakeFunctor>
struct pud_unifier_factory {
    using cycle_detector_t = typename pud_cycle_detector_factory<IGlobalize, IMakeFunctor>::cycle_detector_t;
    using unifier_t =
        pud_unifier<IGlobalize, ubs_bind_map, ubs_bind_map, ubs_bind_map, cycle_detector_t, IMakeFunctor>;

    pud_unifier_factory(IGlobalize& globalize, IMakeFunctor& make_functor);
    unifier_t make_unifier(ubs_bind_map& bind_map, cycle_detector_t& cycle_detector) const;
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
};

#define PUD_UNIFIER_FACTORY_TEMPLATE template<typename IGlobalize, typename IMakeFunctor>
#define PUD_UNIFIER_FACTORY pud_unifier_factory<IGlobalize, IMakeFunctor>

PUD_UNIFIER_FACTORY_TEMPLATE
PUD_UNIFIER_FACTORY::pud_unifier_factory(IGlobalize& globalize, IMakeFunctor& make_functor)
    : globalize_(globalize)
    , make_functor_(make_functor) {}

PUD_UNIFIER_FACTORY_TEMPLATE
typename PUD_UNIFIER_FACTORY::unifier_t PUD_UNIFIER_FACTORY::make_unifier(
    ubs_bind_map& bind_map, cycle_detector_t& cycle_detector) const {
    return unifier_t{globalize_, bind_map, bind_map, bind_map, cycle_detector, make_functor_};
}

#undef PUD_UNIFIER_FACTORY
#undef PUD_UNIFIER_FACTORY_TEMPLATE

#endif
