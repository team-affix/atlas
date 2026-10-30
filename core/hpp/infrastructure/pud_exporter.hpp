#ifndef PUD_EXPORTER_HPP
#define PUD_EXPORTER_HPP

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/moduled_expr.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/ubs_class.hpp"

template<
    typename IGlobalize,
    typename IMakeFunctor,
    typename IMakeVar,
    typename IFind,
    typename IGetGlobalFunctorIdFromLocalFunctorId,
    typename IContainsModule,
    typename IContainsGlobalFunctorId,
    typename IGetLocalFunctorIdFromGlobalFunctorId,
    typename IContainsNamedFunctorId,
    typename IGetNameByGlobalFunctorId,
    typename IGetModule,
    typename IInsertLink>
struct pud_exporter {
    pud_exporter(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IFind& find,
        IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
        IContainsModule& contains_module,
        IContainsGlobalFunctorId& contains_global_functor_id,
        IGetLocalFunctorIdFromGlobalFunctorId& get_local_functor_id,
        IContainsNamedFunctorId& contains_named_functor_id,
        IGetNameByGlobalFunctorId& get_name_by_global_functor_id,
        IGetModule& get_module,
        IInsertLink& insert_link,
        uint32_t caller_lvc,
        std::unordered_map<uint32_t, uint32_t>& translations);
    const expr* export_expr(moduled_expr source, pud_module_id accepting_module);
private:
    struct frame {
        moduled_expr functor;
        std::vector<const expr*> args;
    };
    struct exported_hash {
        size_t operator()(const moduled_expr& term) const noexcept;
    };

    std::optional<const expr*> enter(moduled_expr framed, std::vector<frame>& pending);

    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IFind& find_;
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id_;
    IContainsModule& contains_module_;
    IContainsGlobalFunctorId& contains_global_functor_id_;
    IGetLocalFunctorIdFromGlobalFunctorId& get_local_functor_id_;
    IContainsNamedFunctorId& contains_named_functor_id_;
    IGetNameByGlobalFunctorId& get_name_by_global_functor_id_;
    IGetModule& get_module_;
    IInsertLink& insert_link_;
    uint32_t caller_lvc_;
    std::unordered_map<uint32_t, uint32_t>& translations_;
    std::unordered_map<moduled_expr, const expr*, exported_hash> exported_;
};

#define PUD_EXPORTER_TEMPLATE \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFind, \
             typename IGetGlobalFunctorIdFromLocalFunctorId, typename IContainsModule, \
             typename IContainsGlobalFunctorId, typename IGetLocalFunctorIdFromGlobalFunctorId, \
             typename IContainsNamedFunctorId, typename IGetNameByGlobalFunctorId, \
             typename IGetModule, typename IInsertLink>
#define PUD_EXPORTER \
    pud_exporter<IGlobalize, IMakeFunctor, IMakeVar, IFind, IGetGlobalFunctorIdFromLocalFunctorId, \
        IContainsModule, IContainsGlobalFunctorId, IGetLocalFunctorIdFromGlobalFunctorId, \
        IContainsNamedFunctorId, IGetNameByGlobalFunctorId, IGetModule, IInsertLink>

PUD_EXPORTER_TEMPLATE
PUD_EXPORTER::pud_exporter(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IFind& find,
    IGetGlobalFunctorIdFromLocalFunctorId& get_global_functor_id,
    IContainsModule& contains_module,
    IContainsGlobalFunctorId& contains_global_functor_id,
    IGetLocalFunctorIdFromGlobalFunctorId& get_local_functor_id,
    IContainsNamedFunctorId& contains_named_functor_id,
    IGetNameByGlobalFunctorId& get_name_by_global_functor_id,
    IGetModule& get_module,
    IInsertLink& insert_link,
    uint32_t caller_lvc,
    std::unordered_map<uint32_t, uint32_t>& translations)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , find_(find)
    , get_global_functor_id_(get_global_functor_id)
    , contains_module_(contains_module)
    , contains_global_functor_id_(contains_global_functor_id)
    , get_local_functor_id_(get_local_functor_id)
    , contains_named_functor_id_(contains_named_functor_id)
    , get_name_by_global_functor_id_(get_name_by_global_functor_id)
    , get_module_(get_module)
    , insert_link_(insert_link)
    , caller_lvc_(caller_lvc)
    , translations_(translations)
    , exported_() {}

PUD_EXPORTER_TEMPLATE
const expr* PUD_EXPORTER::export_expr(moduled_expr source, pud_module_id accepting_module) {
    std::vector<frame> pending;
    std::optional<const expr*> finished = enter(source, pending);
    while (!pending.empty()) {
        frame& top = pending.back();
        if (finished.has_value()) {
            top.args.push_back(finished.value());
            finished = std::nullopt;
        }
        const expr::functor& functor = std::get<expr::functor>(top.functor.skeleton->content);
        if (top.args.size() == functor.args.size()) {
            const bool source_module_is_linked = contains_module_.contains_module(top.functor.module);
            const uint32_t global_functor_id = source_module_is_linked
                ? get_global_functor_id_.get_global_functor_id_from_local_functor_id(top.functor.module, functor.id)
                : functor.id;
            const bool accepting_module_has_functor = contains_global_functor_id_.contains_global_functor_id(
                accepting_module, global_functor_id);
            uint32_t local_functor_id = global_functor_id;
            if (accepting_module_has_functor) {
                local_functor_id = get_local_functor_id_.get_local_functor_id_from_global_functor_id(
                    accepting_module, global_functor_id);
            } else if (!source_module_is_linked) {
                local_functor_id = global_functor_id;
            } else if (get_module_.get_module(accepting_module).functor_names().empty()
                || !contains_named_functor_id_.contains_named_functor_id(global_functor_id)) {
                insert_link_.insert_link(accepting_module, global_functor_id, global_functor_id);
            } else {
                const std::string& name = get_name_by_global_functor_id_.get_name_by_global_functor_id(global_functor_id);
                local_functor_id = get_module_.get_module(accepting_module).append_functor_name(name);
                insert_link_.insert_link(accepting_module, local_functor_id, global_functor_id);
            }
            finished = make_functor_.make_functor(local_functor_id, top.args);
            exported_.emplace(top.functor, finished.value());
            pending.pop_back();
            continue;
        }
        finished = enter(
            moduled_expr{functor.args.at(top.args.size()), top.functor.frame_offset, top.functor.module}, pending);
    }
    return finished.value();
}

PUD_EXPORTER_TEMPLATE
std::optional<const expr*> PUD_EXPORTER::enter(moduled_expr framed, std::vector<frame>& pending) {
    if (const expr::var* var = std::get_if<expr::var>(&framed.skeleton->content)) {
        const ubs_class var_class = find_.find(globalize_.globalize(framed.frame_offset, var->index));
        if (var_class.rep < caller_lvc_)
            return make_var_.make_var(var_class.rep);
        if (!var_class.value.has_value()) {
            const uint32_t next_fresh = caller_lvc_ + static_cast<uint32_t>(translations_.size());
            return make_var_.make_var(translations_.try_emplace(var_class.rep, next_fresh).first->second);
        }
        const expr::functor& functor = *var_class.value->functor;
        const expr* reconstructed = make_functor_.make_functor(functor.id, functor.args);
        framed = moduled_expr{reconstructed, var_class.value->frame_offset, var_class.value->module};
    }
    if (const auto known = exported_.find(framed); known != exported_.end())
        return known->second;
    pending.push_back(frame{framed, {}});
    return std::nullopt;
}

PUD_EXPORTER_TEMPLATE
size_t PUD_EXPORTER::exported_hash::operator()(const moduled_expr& term) const noexcept {
    const size_t skeleton_hash = std::hash<const expr*>{}(term.skeleton);
    const size_t mixed = skeleton_hash ^ (std::hash<uint32_t>{}(term.frame_offset) + 0x9e3779b97f4a7c15ull
        + (skeleton_hash << 6) + (skeleton_hash >> 2));
    return mixed ^ (std::hash<pud_module_id>{}(term.module) + 0x9e3779b97f4a7c15ull + (mixed << 6) + (mixed >> 2));
}

#undef PUD_EXPORTER
#undef PUD_EXPORTER_TEMPLATE

#endif
