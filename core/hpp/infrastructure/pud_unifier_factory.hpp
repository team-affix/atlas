#ifndef PUD_UNIFIER_FACTORY_HPP
#define PUD_UNIFIER_FACTORY_HPP

#include "infrastructure/pud_cycle_detector_factory.hpp"
#include "infrastructure/pud_unifier.hpp"
#include "infrastructure/ubs_bind_map.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IDemodule, typename IDemoduleFunctor>
struct pud_unifier_factory {
    using cycle_detector_t = typename pud_cycle_detector_factory<IGlobalize, IDemoduleFunctor>::cycle_detector_t;
    using unifier_t = pud_unifier<
        IGlobalize, ubs_bind_map, ubs_bind_map, ubs_bind_map, cycle_detector_t, IMakeFunctor, IDemodule>;

    pud_unifier_factory(IGlobalize& globalize, IMakeFunctor& make_functor, IDemodule& demodule);
    unifier_t make_unifier(ubs_bind_map& bind_map, cycle_detector_t& cycle_detector) const;
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IDemodule& demodule_;
};

#define PUD_UNIFIER_FACTORY_TEMPLATE \
    template<typename IGlobalize, typename IMakeFunctor, typename IDemodule, typename IDemoduleFunctor>
#define PUD_UNIFIER_FACTORY pud_unifier_factory<IGlobalize, IMakeFunctor, IDemodule, IDemoduleFunctor>

PUD_UNIFIER_FACTORY_TEMPLATE
PUD_UNIFIER_FACTORY::pud_unifier_factory(IGlobalize& globalize, IMakeFunctor& make_functor, IDemodule& demodule)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , demodule_(demodule) {}

PUD_UNIFIER_FACTORY_TEMPLATE
typename PUD_UNIFIER_FACTORY::unifier_t PUD_UNIFIER_FACTORY::make_unifier(
    ubs_bind_map& bind_map, cycle_detector_t& cycle_detector) const {
    return unifier_t{globalize_, bind_map, bind_map, bind_map, cycle_detector, make_functor_, demodule_};
}

#undef PUD_UNIFIER_FACTORY
#undef PUD_UNIFIER_FACTORY_TEMPLATE

#endif
