#ifndef PUD_FUNCTOR_RELINKER_HPP
#define PUD_FUNCTOR_RELINKER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include "value_objects/pud_module_id.hpp"

template<
    typename IGetGlobalFunctorIdFromLocalFunctorId,
    typename IFindLocalFunctorId,
    typename IGetNameByGlobalFunctorId,
    typename IAppendFunctorName,
    typename IInsertLink>
struct pud_functor_relinker {
    pud_functor_relinker(
        IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
        IFindLocalFunctorId& find_local_functor_id,
        IGetNameByGlobalFunctorId& get_name_by_global_functor_id,
        IAppendFunctorName& append_functor_name,
        IInsertLink& insert_link);
    uint32_t relink(pud_module_id from_module, uint32_t local_functor_id, pud_module_id to_module);
private:
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id_;
    IFindLocalFunctorId& find_local_functor_id_;
    IGetNameByGlobalFunctorId& get_name_by_global_functor_id_;
    IAppendFunctorName& append_functor_name_;
    IInsertLink& insert_link_;
};

#define PUD_FUNCTOR_RELINKER_TEMPLATE                                                                  \
    template<typename IGetGlobalFunctorIdFromLocalFunctorId, typename IFindLocalFunctorId,            \
             typename IGetNameByGlobalFunctorId, typename IAppendFunctorName, typename IInsertLink>
#define PUD_FUNCTOR_RELINKER                                                                           \
    pud_functor_relinker<IGetGlobalFunctorIdFromLocalFunctorId, IFindLocalFunctorId,                  \
        IGetNameByGlobalFunctorId, IAppendFunctorName, IInsertLink>

PUD_FUNCTOR_RELINKER_TEMPLATE
PUD_FUNCTOR_RELINKER::pud_functor_relinker(
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
    IFindLocalFunctorId& find_local_functor_id,
    IGetNameByGlobalFunctorId& get_name_by_global_functor_id,
    IAppendFunctorName& append_functor_name,
    IInsertLink& insert_link)
    : get_global_functor_id_(get_global_functor_id)
    , find_local_functor_id_(find_local_functor_id)
    , get_name_by_global_functor_id_(get_name_by_global_functor_id)
    , append_functor_name_(append_functor_name)
    , insert_link_(insert_link) {}

PUD_FUNCTOR_RELINKER_TEMPLATE
uint32_t PUD_FUNCTOR_RELINKER::relink(pud_module_id from_module, uint32_t local_functor_id, pud_module_id to_module) {
    const uint32_t global_functor_id =
        get_global_functor_id_.get_global_functor_id_from_local_functor_id(from_module, local_functor_id);
    const std::optional<uint32_t> linked = find_local_functor_id_.find_local_functor_id(to_module, global_functor_id);
    if (linked.has_value())
        return linked.value();
    const std::string& name = get_name_by_global_functor_id_.get_name_by_global_functor_id(global_functor_id);
    const uint32_t appended = append_functor_name_.append_functor_name(to_module, name);
    insert_link_.insert_link(to_module, appended, global_functor_id);
    return appended;
}

#undef PUD_FUNCTOR_RELINKER
#undef PUD_FUNCTOR_RELINKER_TEMPLATE

#endif
