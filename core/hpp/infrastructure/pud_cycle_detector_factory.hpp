#ifndef PUD_CYCLE_DETECTOR_FACTORY_HPP
#define PUD_CYCLE_DETECTOR_FACTORY_HPP

#include "infrastructure/pud_cycle_detector.hpp"

template<typename IGlobalize, typename BindMap>
struct pud_cycle_detector_factory {
    pud_cycle_detector_factory(IGlobalize& globalize);
    pud_cycle_detector<IGlobalize, BindMap, BindMap> make(BindMap& bind_map) const;
private:
    IGlobalize& globalize_;
};

template<typename IGlobalize, typename BindMap>
pud_cycle_detector_factory<IGlobalize, BindMap>::pud_cycle_detector_factory(IGlobalize& globalize)
    : globalize_(globalize) {}

template<typename IGlobalize, typename BindMap>
pud_cycle_detector<IGlobalize, BindMap, BindMap>
pud_cycle_detector_factory<IGlobalize, BindMap>::make(BindMap& bind_map) const {
    return pud_cycle_detector<IGlobalize, BindMap, BindMap>{globalize_, bind_map, bind_map};
}

#endif
