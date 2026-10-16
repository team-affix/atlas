#ifndef PUD_UNIFIER_FACTORY_HPP
#define PUD_UNIFIER_FACTORY_HPP

#include "infrastructure/pud_unifier.hpp"

template<typename IGlobalize, typename INormalize, typename BindMap>
struct pud_unifier_factory {
    using unifier_t = pud_unifier<IGlobalize, INormalize, BindMap, BindMap, BindMap>;

    pud_unifier_factory(IGlobalize& globalize);
    unifier_t make(INormalize& normalize, BindMap& bind_map) const;
private:
    IGlobalize& globalize_;
};

template<typename IGlobalize, typename INormalize, typename BindMap>
pud_unifier_factory<IGlobalize, INormalize, BindMap>::pud_unifier_factory(IGlobalize& globalize)
    : globalize_(globalize) {}

template<typename IGlobalize, typename INormalize, typename BindMap>
typename pud_unifier_factory<IGlobalize, INormalize, BindMap>::unifier_t
pud_unifier_factory<IGlobalize, INormalize, BindMap>::make(INormalize& normalize, BindMap& bind_map) const {
    return unifier_t{globalize_, normalize, bind_map, bind_map, bind_map};
}

#endif
