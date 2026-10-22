#ifndef PUD_EXPORTER_HPP
#define PUD_EXPORTER_HPP

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

template<
    typename IGlobalize,
    typename IMakeFunctor,
    typename IMakeVar,
    typename IFind,
    typename IFindRep,
    typename IFindValue>
struct pud_exporter {
    pud_exporter(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IFind& find,
        IFindRep& find_rep,
        IFindValue& find_value,
        uint32_t caller_lvc);
    const expr* export_expr(framed_expr framed);
    uint32_t fresh_var_count() const;
private:
    struct frame {
        framed_expr functor;
        std::vector<const expr*> args;
        std::optional<uint32_t> exported_root;
    };

    const expr* enter(framed_expr framed, std::optional<uint32_t> exported_root, std::vector<frame>& pending);

    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IFind& find_;
    IFindRep& find_rep_;
    IFindValue& find_value_;
    uint32_t caller_lvc_;
    std::unordered_map<uint32_t, uint32_t> fresh_vars_;
    std::unordered_map<uint32_t, const expr*> exported_classes_;
};

#define PUD_EXPORTER_TEMPLATE                                                                 \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFind,  \
             typename IFindRep, typename IFindValue>
#define PUD_EXPORTER pud_exporter<IGlobalize, IMakeFunctor, IMakeVar, IFind, IFindRep, IFindValue>

PUD_EXPORTER_TEMPLATE
PUD_EXPORTER::pud_exporter(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IFind& find,
    IFindRep& find_rep,
    IFindValue& find_value,
    uint32_t caller_lvc)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , find_(find)
    , find_rep_(find_rep)
    , find_value_(find_value)
    , caller_lvc_(caller_lvc)
    , fresh_vars_()
    , exported_classes_() {}

PUD_EXPORTER_TEMPLATE
const expr* PUD_EXPORTER::export_expr(framed_expr framed) {
    std::vector<frame> pending;
    const expr* finished = enter(framed, std::nullopt, pending);
    while (!pending.empty()) {
        frame& top = pending.back();
        if (finished) {
            top.args.push_back(finished);
            finished = nullptr;
        }
        const expr::functor& functor = std::get<expr::functor>(top.functor.skeleton->content);
        if (top.args.size() == functor.args.size()) {
            finished = make_functor_.make_functor(functor.id, top.args);
            if (top.exported_root.has_value())
                exported_classes_.emplace(top.exported_root.value(), finished);
            pending.pop_back();
            continue;
        }
        const framed_expr next_arg{functor.args.at(top.args.size()), top.functor.frame_offset};
        finished = enter(next_arg, std::nullopt, pending);
    }
    return finished;
}

PUD_EXPORTER_TEMPLATE
uint32_t PUD_EXPORTER::fresh_var_count() const {
    return static_cast<uint32_t>(fresh_vars_.size());
}

PUD_EXPORTER_TEMPLATE
const expr* PUD_EXPORTER::enter(
    framed_expr framed, std::optional<uint32_t> exported_root, std::vector<frame>& pending) {
    if (const expr::var* var = std::get_if<expr::var>(&framed.skeleton->content)) {
        const uint32_t global = globalize_.globalize(framed.frame_offset, var->index);
        const uint32_t rep = find_rep_.find_rep(global);
        if (rep < caller_lvc_)
            return make_var_.make_var(rep);
        const uint32_t root = find_.find(global);
        if (const auto exported = exported_classes_.find(root); exported != exported_classes_.end())
            return exported->second;
        const std::optional<framed_expr> value = find_value_.find_value(root);
        if (!value.has_value()) {
            const auto [fresh, _] = fresh_vars_.try_emplace(root, caller_lvc_ + static_cast<uint32_t>(fresh_vars_.size()));
            return make_var_.make_var(fresh->second);
        }
        framed = value.value();
        exported_root = root;
    }
    pending.push_back(frame{framed, {}, exported_root});
    return nullptr;
}

#undef PUD_EXPORTER
#undef PUD_EXPORTER_TEMPLATE

#endif
