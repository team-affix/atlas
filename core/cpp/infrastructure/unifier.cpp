#include "infrastructure/unifier.hpp"

unifier::unifier(i_bind_map& bm)
    : bind_map(bm) {}

coroutine<uint32_t, bool> unifier::unify(const expr* lhs, const expr* rhs) {
        // WHNF the lhs and rhs
    lhs = bind_map.whnf(lhs);
    rhs = bind_map.whnf(rhs);

    // get the lhs and rhs var handles if they are variables
    const expr::var* lv = std::get_if<expr::var>(&lhs->content);
    const expr::var* rv = std::get_if<expr::var>(&rhs->content);

    if (lv)
        co_yield lv->index;
    if (rv)
        co_yield rv->index;

    // If both sides are variables, bind the younger (higher index) to the older
    if (lv && rv) {
        if (lv->index == rv->index)
            co_return true;
        const auto [young, target] = lv->index > rv->index
            ? std::pair{lv, rhs}
            : std::pair{rv, lhs};
        if (occurs_check(young->index, target))
            co_return false;
        bind_map.bind(young->index, target);
        co_return true;
    }

    // If one side is a variable, bind it to the other side
    const auto [v, other_e] = lv
        ? std::pair{lv, rhs}
        : std::pair{rv, lhs};
    if (v) {
        if (occurs_check(v->index, other_e))
            co_return false;
        bind_map.bind(v->index, other_e);
        co_return true;
    }

    // At this point, they are both functors
    const expr::functor& lf = std::get<expr::functor>(lhs->content);
    const expr::functor& rf = std::get<expr::functor>(rhs->content);
    if (lf.name != rf.name || lf.args.size() != rf.args.size())
        co_return false;

    // unify the children
    for (size_t i = 0; i < lf.args.size(); ++i) {
        auto unify_child_task = unify(lf.args[i], rf.args[i]);

        // wait for the child unify to complete
        while (!unify_child_task.done()) {
            unify_child_task.resume();
            if (unify_child_task.has_yield()) {
                co_yield unify_child_task.consume_yield();
            }
        }

        // if the child unify failed, return false
        if (!unify_child_task.result())
            co_return false;
    }
    co_return true;
}

bool unifier::occurs_check(uint32_t index, const expr* key) {
    key = bind_map.whnf(key);

    if (const expr::var* var = std::get_if<expr::var>(&key->content))
        return var->index == index;

    if (const expr::functor* f = std::get_if<expr::functor>(&key->content)) {
        for (const expr* arg : f->args)
            if (occurs_check(index, arg))
                return true;
        return false;
    }

    return false;
}
