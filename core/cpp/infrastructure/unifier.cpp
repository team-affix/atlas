#include "infrastructure/locator.hpp"
#include "infrastructure/unifier.hpp"

unifier::unifier(locator& loc, i_bind_map& bm)
    : bind_map(bm), globalizer_(loc.locate<i_globalizer>()) {}

coroutine<uint32_t, bool> unifier::unify(framed_expr lhs, framed_expr rhs) {
    lhs = bind_map.whnf(lhs);
    rhs = bind_map.whnf(rhs);

    const expr::var* lv = std::get_if<expr::var>(&lhs.skeleton->content);
    const expr::var* rv = std::get_if<expr::var>(&rhs.skeleton->content);

    const uint32_t lv_global = lv ? globalizer_.globalize(lhs.frame_offset, lv->index) : 0;
    const uint32_t rv_global = rv ? globalizer_.globalize(rhs.frame_offset, rv->index) : 0;

    if (lv && rv && lv_global == rv_global)
        co_return true;

    // Bind the younger (higher global key) to the older.
    if (lv && rv) {
        const auto [young_global, target] = lv_global > rv_global
            ? std::pair{lv_global, rhs}
            : std::pair{rv_global, lhs};
        if (occurs_check(young_global, target))
            co_return false;
        co_yield lv_global;
        co_yield rv_global;
        bind_map.bind(young_global, target);
        co_return true;
    }

    // One side is a variable.
    if (lv || rv) {
        const uint32_t var_global = lv ? lv_global : rv_global;
        const framed_expr other = lv ? rhs : lhs;
        if (occurs_check(var_global, other))
            co_return false;
        co_yield var_global;
        bind_map.bind(var_global, other);
        co_return true;
    }

    // Both are functors.
    const expr::functor& lf = std::get<expr::functor>(lhs.skeleton->content);
    const expr::functor& rf = std::get<expr::functor>(rhs.skeleton->content);
    if (lf.id != rf.id || lf.args.size() != rf.args.size())
        co_return false;

    for (size_t i = 0; i < lf.args.size(); ++i) {
        auto child_task = unify({lf.args[i], lhs.frame_offset}, {rf.args[i], rhs.frame_offset});
        while (!child_task.done()) {
            child_task.resume();
            if (child_task.has_yield())
                co_yield child_task.consume_yield();
        }
        if (!child_task.result())
            co_return false;
    }
    co_return true;
}

bool unifier::occurs_check(uint32_t global_key, framed_expr fe) {
    fe = bind_map.whnf(fe);

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
