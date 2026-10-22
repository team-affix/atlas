#ifndef PUD_UNIFIER_FACTORY_HPP
#define PUD_UNIFIER_FACTORY_HPP

#include "infrastructure/pud_unifier.hpp"

template<typename IGlobalize, typename CycleDetector, typename BindMap>
struct pud_unifier_factory {
    using unifier_t = pud_unifier<IGlobalize, BindMap, BindMap, BindMap, BindMap, CycleDetector>;

    pud_unifier_factory(IGlobalize& globalize);
    unifier_t make(CycleDetector& cycle_detector, BindMap& bind_map) const;
private:
    IGlobalize& globalize_;
};

template<typename IGlobalize, typename CycleDetector, typename BindMap>
pud_unifier_factory<IGlobalize, CycleDetector, BindMap>::pud_unifier_factory(IGlobalize& globalize)
    : globalize_(globalize) {}

template<typename IGlobalize, typename CycleDetector, typename BindMap>
typename pud_unifier_factory<IGlobalize, CycleDetector, BindMap>::unifier_t
pud_unifier_factory<IGlobalize, CycleDetector, BindMap>::make(CycleDetector& cycle_detector, BindMap& bind_map) const {
    return unifier_t{globalize_, bind_map, bind_map, bind_map, bind_map, cycle_detector};
}

#endif
