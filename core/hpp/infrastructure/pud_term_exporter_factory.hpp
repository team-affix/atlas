#ifndef PUD_TERM_EXPORTER_FACTORY_HPP
#define PUD_TERM_EXPORTER_FACTORY_HPP

#include <cstdint>
#include <unordered_map>
#include "infrastructure/pud_term_exporter.hpp"
#include "infrastructure/ubs_bind_map.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IRelinkFunctor>
struct pud_term_exporter_factory {
    using exporter_t = pud_term_exporter<IGlobalize, IMakeFunctor, IMakeVar, ubs_bind_map, IRelinkFunctor>;

    pud_term_exporter_factory(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IRelinkFunctor& relink_functor);
    exporter_t make_exporter(
        ubs_bind_map& bind_map,
        uint32_t caller_lvc,
        std::unordered_map<uint32_t, uint32_t>& translations) const;
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IRelinkFunctor& relink_functor_;
};

#define PUD_TERM_EXPORTER_FACTORY_TEMPLATE \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IRelinkFunctor>
#define PUD_TERM_EXPORTER_FACTORY pud_term_exporter_factory<IGlobalize, IMakeFunctor, IMakeVar, IRelinkFunctor>

PUD_TERM_EXPORTER_FACTORY_TEMPLATE
PUD_TERM_EXPORTER_FACTORY::pud_term_exporter_factory(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IRelinkFunctor& relink_functor)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , relink_functor_(relink_functor) {}

PUD_TERM_EXPORTER_FACTORY_TEMPLATE
typename PUD_TERM_EXPORTER_FACTORY::exporter_t PUD_TERM_EXPORTER_FACTORY::make_exporter(
    ubs_bind_map& bind_map,
    uint32_t caller_lvc,
    std::unordered_map<uint32_t, uint32_t>& translations) const {
    return exporter_t{globalize_, make_functor_, make_var_, bind_map, relink_functor_, caller_lvc, translations};
}

#undef PUD_TERM_EXPORTER_FACTORY
#undef PUD_TERM_EXPORTER_FACTORY_TEMPLATE

#endif
