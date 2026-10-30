#ifndef PUD_NORMALIZER_FACTORY_HPP
#define PUD_NORMALIZER_FACTORY_HPP

#include "infrastructure/pud_normalizer.hpp"
#include "infrastructure/ubs_bind_map.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IDemoduleFunctor>
struct pud_normalizer_factory {
    using normalizer_t = pud_normalizer<IGlobalize, IMakeFunctor, IMakeVar, ubs_bind_map, IDemoduleFunctor>;

    pud_normalizer_factory(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IDemoduleFunctor& demodule_functor);
    normalizer_t make_normalizer(ubs_bind_map& bind_map) const;
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IDemoduleFunctor& demodule_functor_;
};

#define PUD_NORMALIZER_FACTORY_TEMPLATE \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IDemoduleFunctor>
#define PUD_NORMALIZER_FACTORY pud_normalizer_factory<IGlobalize, IMakeFunctor, IMakeVar, IDemoduleFunctor>

PUD_NORMALIZER_FACTORY_TEMPLATE
PUD_NORMALIZER_FACTORY::pud_normalizer_factory(
    IGlobalize& globalize, IMakeFunctor& make_functor, IMakeVar& make_var, IDemoduleFunctor& demodule_functor)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , demodule_functor_(demodule_functor) {}

PUD_NORMALIZER_FACTORY_TEMPLATE
typename PUD_NORMALIZER_FACTORY::normalizer_t PUD_NORMALIZER_FACTORY::make_normalizer(ubs_bind_map& bind_map) const {
    return normalizer_t{globalize_, make_functor_, make_var_, bind_map, demodule_functor_};
}

#undef PUD_NORMALIZER_FACTORY
#undef PUD_NORMALIZER_FACTORY_TEMPLATE

#endif
