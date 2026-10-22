#ifndef GLOBAL_NORMALIZER_HPP
#define GLOBAL_NORMALIZER_HPP

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFindRep, typename IFindValue>
struct global_normalizer {
    global_normalizer(
        IGlobalize& globalize,
        IMakeFunctor& make_functor,
        IMakeVar& make_var,
        IFindRep& find_rep,
        IFindValue& find_value);
    const expr* normalize(framed_expr fe);

private:
    struct frame {
        framed_expr functor;
        std::vector<const expr*> args;
    };

    const expr* enter(framed_expr fe, std::vector<frame>& pending);

    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IMakeVar& make_var_;
    IFindRep& find_rep_;
    IFindValue& find_value_;
};

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFindRep, typename IFindValue>
global_normalizer<IGlobalize, IMakeFunctor, IMakeVar, IFindRep, IFindValue>::global_normalizer(
    IGlobalize& globalize,
    IMakeFunctor& make_functor,
    IMakeVar& make_var,
    IFindRep& find_rep,
    IFindValue& find_value)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , find_rep_(find_rep)
    , find_value_(find_value) {}

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFindRep, typename IFindValue>
const expr* global_normalizer<IGlobalize, IMakeFunctor, IMakeVar, IFindRep, IFindValue>::normalize(framed_expr fe) {
    std::vector<frame> pending;
    const expr* finished = enter(fe, pending);
    while (!pending.empty()) {
        frame& top = pending.back();
        if (finished) {
            top.args.push_back(finished);
            finished = nullptr;
        }
        const expr::functor& functor = std::get<expr::functor>(top.functor.skeleton->content);
        if (top.args.size() == functor.args.size()) {
            finished = make_functor_.make_functor(functor.id, top.args);
            pending.pop_back();
            continue;
        }
        const framed_expr next_arg{functor.args.at(top.args.size()), top.functor.frame_offset};
        finished = enter(next_arg, pending);
    }
    return finished;
}

template<typename IGlobalize, typename IMakeFunctor, typename IMakeVar, typename IFindRep, typename IFindValue>
const expr* global_normalizer<IGlobalize, IMakeFunctor, IMakeVar, IFindRep, IFindValue>::enter(framed_expr fe, std::vector<frame>& pending) {
    if (const expr::var* var = std::get_if<expr::var>(&fe.skeleton->content)) {
        const uint32_t global = globalize_.globalize(fe.frame_offset, var->index);
        const std::optional<framed_expr> value = find_value_.find_value(global);
        if (!value.has_value())
            return make_var_.make_var(find_rep_.find_rep(global));
        fe = value.value();
    }
    pending.push_back(frame{fe, {}});
    return nullptr;
}

#endif
