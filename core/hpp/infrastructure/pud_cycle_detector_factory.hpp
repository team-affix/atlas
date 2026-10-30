#ifndef PUD_CYCLE_DETECTOR_FACTORY_HPP
#define PUD_CYCLE_DETECTOR_FACTORY_HPP

#include "infrastructure/pud_cycle_detector.hpp"
#include "infrastructure/ubs_bind_map.hpp"

template<typename IGlobalize, typename IDemoduleFunctor>
struct pud_cycle_detector_factory {
    using cycle_detector_t = pud_cycle_detector<IGlobalize, ubs_bind_map, IDemoduleFunctor>;

    pud_cycle_detector_factory(IGlobalize& globalize, IDemoduleFunctor& demodule_functor);
    cycle_detector_t make_cycle_detector(ubs_bind_map& bind_map) const;
private:
    IGlobalize& globalize_;
    IDemoduleFunctor& demodule_functor_;
};

#define PUD_CYCLE_DETECTOR_FACTORY_TEMPLATE template<typename IGlobalize, typename IDemoduleFunctor>
#define PUD_CYCLE_DETECTOR_FACTORY pud_cycle_detector_factory<IGlobalize, IDemoduleFunctor>

PUD_CYCLE_DETECTOR_FACTORY_TEMPLATE
PUD_CYCLE_DETECTOR_FACTORY::pud_cycle_detector_factory(IGlobalize& globalize, IDemoduleFunctor& demodule_functor)
    : globalize_(globalize)
    , demodule_functor_(demodule_functor) {}

PUD_CYCLE_DETECTOR_FACTORY_TEMPLATE
typename PUD_CYCLE_DETECTOR_FACTORY::cycle_detector_t PUD_CYCLE_DETECTOR_FACTORY::make_cycle_detector(
    ubs_bind_map& bind_map) const {
    return cycle_detector_t{globalize_, bind_map, demodule_functor_};
}

#undef PUD_CYCLE_DETECTOR_FACTORY
#undef PUD_CYCLE_DETECTOR_FACTORY_TEMPLATE

#endif
