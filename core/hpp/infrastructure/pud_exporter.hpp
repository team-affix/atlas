#ifndef PUD_EXPORTER_HPP
#define PUD_EXPORTER_HPP

#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/moduled_expr.hpp"
#include "value_objects/moduled_expr_hash.hpp"
#include "value_objects/moduled_functor.hpp"
#include "value_objects/pud_module_id.hpp"
#include "value_objects/ubs_class.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFind, typename IRelinkFunctor>
struct pud_exporter {
    pud_exporter(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IFind& find,
        IRelinkFunctor& relink_functor,
        uint32_t caller_lvc,
        std::unordered_map<uint32_t, uint32_t>& translations);
    const expr* export_expr(moduled_expr source, pud_module_id accepting_module);
private:
    struct frame {
        moduled_expr functor;
        std::vector<const expr*> args;
    };
    std::optional<const expr*> enter(moduled_expr framed, std::vector<frame>& pending);

    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IFind& find_;
    IRelinkFunctor& relink_functor_;
    uint32_t caller_lvc_;
    std::unordered_map<uint32_t, uint32_t>& translations_;
    std::unordered_map<moduled_expr, const expr*, moduled_expr_hash> exported_;
};

#define PUD_EXPORTER_TEMPLATE \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFind, typename IRelinkFunctor>
#define PUD_EXPORTER pud_exporter<IGlobalize, IMakeFunctor, IMakeVar, IFind, IRelinkFunctor>

PUD_EXPORTER_TEMPLATE
PUD_EXPORTER::pud_exporter(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IFind& find,
    IRelinkFunctor& relink_functor,
    uint32_t caller_lvc,
    std::unordered_map<uint32_t, uint32_t>& translations)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , find_(find)
    , relink_functor_(relink_functor)
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
            const uint32_t local_functor_id = relink_functor_.relink(top.functor.module, functor.id, accepting_module);
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

#undef PUD_EXPORTER
#undef PUD_EXPORTER_TEMPLATE

#endif
