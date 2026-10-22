#ifndef PUD_NORMALIZER_FACTORY_HPP
#define PUD_NORMALIZER_FACTORY_HPP

#include "infrastructure/pud_normalizer.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename BindMap>
struct pud_normalizer_factory {
    using normalizer_t = global_normalizer<IGlobalize, IMakeFunctor, IMakeVar, BindMap, BindMap>;

    pud_normalizer_factory(IGlobalize& globalize, IMakeFunctor& make_functor, IMakeVar& make_var);
    normalizer_t make(BindMap& bind_map) const;
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
};

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename BindMap>
pud_normalizer_factory<IGlobalize, IMakeFunctor, IMakeVar, BindMap>::pud_normalizer_factory(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var) {}

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename BindMap>
typename pud_normalizer_factory<IGlobalize, IMakeFunctor, IMakeVar, BindMap>::normalizer_t
pud_normalizer_factory<IGlobalize, IMakeFunctor, IMakeVar, BindMap>::make(BindMap& bind_map) const {
    return normalizer_t{globalize_, make_functor_, make_var_, bind_map, bind_map};
}

#endif
