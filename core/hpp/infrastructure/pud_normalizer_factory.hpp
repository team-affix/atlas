#ifndef PUD_NORMALIZER_FACTORY_HPP
#define PUD_NORMALIZER_FACTORY_HPP

#include "infrastructure/pud_normalizer.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IResolve>
struct pud_normalizer_factory {
    using normalizer_t = global_normalizer<IGlobalize, IMakeFunctor, IResolve>;

    pud_normalizer_factory(IGlobalize& globalize, IMakeFunctor& make_functor);
    normalizer_t make(IResolve& resolve) const;
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
};

template<typename IGlobalize, typename IMakeFunctor, typename IResolve>
pud_normalizer_factory<IGlobalize, IMakeFunctor, IResolve>::pud_normalizer_factory(
    IGlobalize& globalize,
    IMakeFunctor& make_functor)
    : globalize_(globalize)
    , make_functor_(make_functor) {}

template<typename IGlobalize, typename IMakeFunctor, typename IResolve>
typename pud_normalizer_factory<IGlobalize, IMakeFunctor, IResolve>::normalizer_t
pud_normalizer_factory<IGlobalize, IMakeFunctor, IResolve>::make(IResolve& resolve) const {
    return normalizer_t{globalize_, make_functor_, resolve};
}

#endif
