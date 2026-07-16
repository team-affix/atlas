#ifndef UNIFIER_HPP
#define UNIFIER_HPP

#include <cstdint>
#include "infrastructure/coroutine.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/expr.hpp"

template<typename IGlobalize, typename IBindMap>
struct unifier {
    unifier(IGlobalize& g, IBindMap* bm);
    coroutine<uint32_t, bool> unify(framed_expr lhs, framed_expr rhs);
private:
    bool occurs_check(uint32_t global_key, framed_expr);
    IBindMap* bind_map_;
    IGlobalize& globalizer_;
};

template<typename IGlobalize, typename IBindMap>
unifier<IGlobalize, IBindMap>::unifier(IGlobalize& g, IBindMap* bm)
    : bind_map_(bm), globalizer_(g) {}

template<typename IGlobalize, typename IBindMap>
coroutine<uint32_t, bool> unifier<IGlobalize, IBindMap>::unify(framed_expr lhs, framed_expr rhs) {
    lhs = bind_map_->whnf(lhs);
    rhs = bind_map_->whnf(rhs);

    const expr::var* lv = std::get_if<expr::var>(&lhs.skeleton->content);
    const expr::var* rv = std::get_if<expr::var>(&rhs.skeleton->content);

    const uint32_t lv_global = lv ? globalizer_.globalize(lhs.frame_offset, lv->index) : 0;
    const uint32_t rv_global = rv ? globalizer_.globalize(rhs.frame_offset, rv->index) : 0;

    if (lv && rv && lv_global == rv_global)
        co_return true;

    if (lv && rv) {
        const auto [young_global, target] = lv_global > rv_global
            ? std::pair{lv_global, rhs}
            : std::pair{rv_global, lhs};
        if (occurs_check(young_global, target))
            co_return false;
        co_yield lv_global;
        co_yield rv_global;
        bind_map_->bind(young_global, target);
        co_return true;
    }

    if (lv || rv) {
        const uint32_t var_global = lv ? lv_global : rv_global;
        const framed_expr other = lv ? rhs : lhs;
        if (occurs_check(var_global, other))
            co_return false;
        co_yield var_global;
        bind_map_->bind(var_global, other);
        co_return true;
    }

    const expr::functor& lf = std::get<expr::functor>(lhs.skeleton->content);
    const expr::functor& rf = std::get<expr::functor>(rhs.skeleton->content);
    if (lf.id != rf.id || lf.args.size() != rf.args.size())
        co_return false;

    for (size_t i = 0; i < lf.args.size(); ++i) {
        auto child_task = unify({lf.args[i], lhs.frame_offset}, {rf.args[i], rhs.frame_offset});
        while (!child_task.done()) {
            child_task.resume();
            if (child_task.has_yield()) co_yield child_task.consume_yield();
        }
        if (!child_task.result()) co_return false;
    }
    co_return true;
}

template<typename IGlobalize, typename IBindMap>
bool unifier<IGlobalize, IBindMap>::occurs_check(uint32_t global_key, framed_expr fe) {
    fe = bind_map_->whnf(fe);

    if (const expr::var* v = std::get_if<expr::var>(&fe.skeleton->content))
        return globalizer_.globalize(fe.frame_offset, v->index) == global_key;

    if (const expr::functor* f = std::get_if<expr::functor>(&fe.skeleton->content)) {
        for (const expr* arg : f->args)
            if (occurs_check(global_key, {arg, fe.frame_offset}))
                return true;
        return false;
    }

    return false;
}

#endif
