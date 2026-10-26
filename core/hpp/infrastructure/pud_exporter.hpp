#ifndef PUD_EXPORTER_HPP
#define PUD_EXPORTER_HPP

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/framed_expr_hash.hpp"
#include "value_objects/framed_functor.hpp"
#include "value_objects/ubs_class.hpp"

// Rebuilds framed terms for a caller: classes represented below the caller's lvc stay the caller's
// variables, other bound classes are resolved, and other unbound classes become fresh variables
// numbered densely from the lvc. Each fresh variable is recorded in the translations, keyed by the
// representative it replaces, so the translations' size is the number of fresh variables. The
// translations and the rebuilt subterms are shared by every export of one exporter, so every term of
// an inference is rebuilt in one environment.
template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFind>
struct pud_exporter {
    pud_exporter(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IFind& find,
        uint32_t caller_lvc,
        std::unordered_map<uint32_t, uint32_t>& translations);
    const expr* export_expr(framed_expr framed);
private:
    struct frame {
        framed_expr functor;
        std::vector<const expr*> args;
    };

    std::optional<const expr*> enter(framed_expr framed, std::vector<frame>& pending);

    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IFind& find_;
    uint32_t caller_lvc_;
    std::unordered_map<uint32_t, uint32_t>& translations_;
    std::unordered_map<framed_expr, const expr*, framed_expr_hash> exported_;
};

#define PUD_EXPORTER_TEMPLATE \
    template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFind>
#define PUD_EXPORTER pud_exporter<IGlobalize, IMakeFunctor, IMakeVar, IFind>

PUD_EXPORTER_TEMPLATE
PUD_EXPORTER::pud_exporter(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IFind& find,
    uint32_t caller_lvc,
    std::unordered_map<uint32_t, uint32_t>& translations)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , find_(find)
    , caller_lvc_(caller_lvc)
    , translations_(translations)
    , exported_() {}

PUD_EXPORTER_TEMPLATE
const expr* PUD_EXPORTER::export_expr(framed_expr framed) {
    std::vector<frame> pending;
    std::optional<const expr*> finished = enter(framed, pending);
    while (!pending.empty()) {
        frame& top = pending.back();
        if (finished.has_value()) {
            top.args.push_back(finished.value());
            finished = std::nullopt;
        }
        const expr::functor& functor = std::get<expr::functor>(top.functor.skeleton->content);
        if (top.args.size() == functor.args.size()) {
            finished = make_functor_.make_functor(functor.id, top.args);
            exported_.emplace(top.functor, finished.value());
            pending.pop_back();
            continue;
        }
        finished = enter(framed_expr{functor.args.at(top.args.size()), top.functor.frame_offset}, pending);
    }
    return finished.value();
}

// Returns the exported term when it is already known, or pushes the functor still to be rebuilt.
PUD_EXPORTER_TEMPLATE
std::optional<const expr*> PUD_EXPORTER::enter(framed_expr framed, std::vector<frame>& pending) {
    if (const expr::var* var = std::get_if<expr::var>(&framed.skeleton->content)) {
        const ubs_class var_class = find_.find(globalize_.globalize(framed.frame_offset, var->index));
        if (var_class.rep < caller_lvc_)
            return make_var_.make_var(var_class.rep);
        if (!var_class.value.has_value()) {
            const uint32_t next_fresh = caller_lvc_ + static_cast<uint32_t>(translations_.size());
            return make_var_.make_var(translations_.try_emplace(var_class.rep, next_fresh).first->second);
        }
        const expr::functor& f = *var_class.value->functor;
        const expr* reconstructed = make_functor_.make_functor(f.id, f.args);
        framed = framed_expr{reconstructed, var_class.value->frame_offset};
    }
    if (const auto known = exported_.find(framed); known != exported_.end())
        return known->second;
    pending.push_back(frame{framed, {}});
    return std::nullopt;
}

#undef PUD_EXPORTER
#undef PUD_EXPORTER_TEMPLATE

#endif
