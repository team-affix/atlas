#ifndef PUD_EXPORTER_FACTORY_HPP
#define PUD_EXPORTER_FACTORY_HPP

#include <cstdint>
#include <unordered_map>
#include "infrastructure/pud_exporter.hpp"
#include "infrastructure/ubs_bind_map.hpp"

template<
    typename IGlobalize,
    typename IMakeFunctor,
    typename IMakeVar,
    typename IGetGlobalFunctorIdFromLocalFunctorId,
    typename IContainsModule,
    typename IContainsGlobalFunctorId,
    typename IGetLocalFunctorIdFromGlobalFunctorId,
    typename IContainsNamedFunctorId,
    typename IGetNameByGlobalFunctorId,
    typename IGetModule,
    typename IInsertLink>
struct pud_exporter_factory {
    using exporter_t = pud_exporter<
        IGlobalize, IMakeFunctor, IMakeVar, ubs_bind_map, IGetGlobalFunctorIdFromLocalFunctorId,
        IContainsModule, IContainsGlobalFunctorId, IGetLocalFunctorIdFromGlobalFunctorId,
        IContainsNamedFunctorId, IGetNameByGlobalFunctorId, IGetModule, IInsertLink>;

    pud_exporter_factory(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
        IContainsModule& contains_module,
        IContainsGlobalFunctorId& contains_global_functor_id,
        IGetLocalFunctorIdFromGlobalFunctorId& get_local_functor_id,
        IContainsNamedFunctorId& contains_named_functor_id,
        IGetNameByGlobalFunctorId& get_name_by_global_functor_id,
        IGetModule& get_module,
        IInsertLink& insert_link);
    exporter_t make_exporter(
        ubs_bind_map& bind_map,
        uint32_t caller_lvc,
        std::unordered_map<uint32_t, uint32_t>& translations) const;
private:
    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id_;
    IContainsModule& contains_module_;
    IContainsGlobalFunctorId& contains_global_functor_id_;
    IGetLocalFunctorIdFromGlobalFunctorId& get_local_functor_id_;
    IContainsNamedFunctorId& contains_named_functor_id_;
    IGetNameByGlobalFunctorId& get_name_by_global_functor_id_;
    IGetModule& get_module_;
    IInsertLink& insert_link_;
};

#define PUD_EXPORTER_FACTORY_TEMPLATE \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, \
             typename IGetGlobalFunctorIdFromLocalFunctorId, typename IContainsModule, \
             typename IContainsGlobalFunctorId, typename IGetLocalFunctorIdFromGlobalFunctorId, \
             typename IContainsNamedFunctorId, typename IGetNameByGlobalFunctorId, \
             typename IGetModule, typename IInsertLink>
#define PUD_EXPORTER_FACTORY \
    pud_exporter_factory<IGlobalize, IMakeFunctor, IMakeVar, IGetGlobalFunctorIdFromLocalFunctorId, \
        IContainsModule, IContainsGlobalFunctorId, IGetLocalFunctorIdFromGlobalFunctorId, \
        IContainsNamedFunctorId, IGetNameByGlobalFunctorId, IGetModule, IInsertLink>

PUD_EXPORTER_FACTORY_TEMPLATE
PUD_EXPORTER_FACTORY::pud_exporter_factory(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
    IContainsModule& contains_module,
    IContainsGlobalFunctorId& contains_global_functor_id,
    IGetLocalFunctorIdFromGlobalFunctorId& get_local_functor_id,
    IContainsNamedFunctorId& contains_named_functor_id,
    IGetNameByGlobalFunctorId& get_name_by_global_functor_id,
    IGetModule& get_module,
    IInsertLink& insert_link)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , get_global_functor_id_(get_global_functor_id)
    , contains_module_(contains_module)
    , contains_global_functor_id_(contains_global_functor_id)
    , get_local_functor_id_(get_local_functor_id)
    , contains_named_functor_id_(contains_named_functor_id)
    , get_name_by_global_functor_id_(get_name_by_global_functor_id)
    , get_module_(get_module)
    , insert_link_(insert_link) {}

PUD_EXPORTER_FACTORY_TEMPLATE
typename PUD_EXPORTER_FACTORY::exporter_t PUD_EXPORTER_FACTORY::make_exporter(
    ubs_bind_map& bind_map,
    uint32_t caller_lvc,
    std::unordered_map<uint32_t, uint32_t>& translations) const {
    return exporter_t{
        globalize_, make_functor_, make_var_, bind_map, get_global_functor_id_, contains_module_,
        contains_global_functor_id_, get_local_functor_id_, contains_named_functor_id_,
        get_name_by_global_functor_id_, get_module_, insert_link_, caller_lvc, translations};
}

#undef PUD_EXPORTER_FACTORY
#undef PUD_EXPORTER_FACTORY_TEMPLATE

#endif
