#ifndef PUD_EXPORTER_FACTORY_HPP
#define PUD_EXPORTER_FACTORY_HPP

#include <cstdint>
#include <unordered_map>
#include "infrastructure/pud_exporter.hpp"
#include "infrastructure/ubs_bind_map.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar>
struct pud_exporter_factory {
    using exporter_t = pud_exporter<IGlobalize, IMakeFunctor, IMakeVar, ubs_bind_map>;

    pud_exporter_factory(IGlobalize& globalize, IMakeFunctor& make_functor, IMakeVar& make_var);
    exporter_t make_exporter(
        ubs_bind_map& bind_map,
        uint32_t caller_lvc,
        std::unordered_map<uint32_t, uint32_t>& translations) const;
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
};

#define PUD_EXPORTER_FACTORY_TEMPLATE template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar>
#define PUD_EXPORTER_FACTORY pud_exporter_factory<IGlobalize, IMakeFunctor, IMakeVar>

PUD_EXPORTER_FACTORY_TEMPLATE
PUD_EXPORTER_FACTORY::pud_exporter_factory(IGlobalize& globalize, IMakeFunctor& make_functor, IMakeVar& make_var)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var) {}

PUD_EXPORTER_FACTORY_TEMPLATE
typename PUD_EXPORTER_FACTORY::exporter_t PUD_EXPORTER_FACTORY::make_exporter(
    ubs_bind_map& bind_map,
    uint32_t caller_lvc,
    std::unordered_map<uint32_t, uint32_t>& translations) const {
    return exporter_t{globalize_, make_functor_, make_var_, bind_map, caller_lvc, translations};
}

#undef PUD_EXPORTER_FACTORY
#undef PUD_EXPORTER_FACTORY_TEMPLATE

#endif
