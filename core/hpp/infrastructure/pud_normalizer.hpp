#ifndef GLOBAL_NORMALIZER_HPP
#define GLOBAL_NORMALIZER_HPP

#include <variant>
#include <vector>
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

// Iterative, so term depth is bounded by memory rather than by the call stack.
template<typename IGlobalize, typename IMakeFunctor, typename IResolve>
struct global_normalizer {
    global_normalizer(IGlobalize& globalize, IMakeFunctor& make_functor, IResolve& resolve);
    const expr* normalize(framed_expr fe);

private:
    struct frame {
        framed_expr functor;
        std::vector<const expr*> args;
    };

    const expr* enter(framed_expr fe, std::vector<frame>& pending);

    IGlobalize& globalize_;
    IMakeFunctor& make_functor_;
    IResolve& resolve_;
};

template<typename IGlobalize, typename IMakeFunctor, typename IResolve>
global_normalizer<IGlobalize, IMakeFunctor, IResolve>::global_normalizer(
    IGlobalize& globalize, IMakeFunctor& make_functor, IResolve& resolve)
    : globalize_(globalize)
    , make_functor_(make_functor)
    , resolve_(resolve) {}

template<typename IGlobalize, typename IMakeFunctor, typename IResolve>
const expr* global_normalizer<IGlobalize, IMakeFunctor, IResolve>::normalize(framed_expr fe) {
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

// A resolved variable is returned directly; a functor is pushed and nullptr returned.
template<typename IGlobalize, typename IMakeFunctor, typename IResolve>
const expr* global_normalizer<IGlobalize, IMakeFunctor, IResolve>::enter(framed_expr fe, std::vector<frame>& pending) {
    if (const expr::var* var = std::get_if<expr::var>(&fe.skeleton->content)) {
        const expr* value = resolve_.resolve(globalize_.globalize(fe.frame_offset, var->index));
        if (std::holds_alternative<expr::var>(value->content))
            return value;
        fe = {value, 0u};
    }
    pending.push_back(frame{fe, {}});
    return nullptr;
}

#endif
