#include "../../hpp/infrastructure/unifier.hpp"

unifier::unifier(std::unique_ptr<i_bind_map> bm)
    : i_unifier(std::move(bm)) {}

bool unifier::unify(const expr* lhs, const expr* rhs, std::unordered_set<uint32_t>& snk) {
        // WHNF the lhs and rhs
    lhs = bind_map->whnf(lhs);
    rhs = bind_map->whnf(rhs);

    // get the lhs and rhs var handles if they are variables
    const expr::var* lv = std::get_if<expr::var>(&lhs->content);
    const expr::var* rv = std::get_if<expr::var>(&rhs->content);

    // If both sides are variables, bind the younger (higher index) to the older
    if (lv && rv) {
        if (lv->index == rv->index)
            return true;
        const auto [young, target] = lv->index > rv->index
            ? std::pair{lv, rhs}
            : std::pair{rv, lhs};
        if (occurs_check(young->index, target))
            return false;
        bind_map->bind(young->index, target);
        snk.insert(young->index);
        return true;
    }

    // If one side is a variable, bind it to the other side
    const auto [v, other_e] = lv
        ? std::pair{lv, rhs}
        : std::pair{rv, lhs};
    if (v) {
        if (occurs_check(v->index, other_e))
            return false;
        bind_map->bind(v->index, other_e);
        snk.insert(v->index);
        return true;
    }

    // At this point, they are both functors
    const expr::functor& lf = std::get<expr::functor>(lhs->content);
    const expr::functor& rf = std::get<expr::functor>(rhs->content);
    if (lf.name != rf.name || lf.args.size() != rf.args.size())
        return false;
    for (size_t i = 0; i < lf.args.size(); ++i)
        if (!unify(lf.args[i], rf.args[i], snk))
            return false;
    return true;
}

bool unifier::occurs_check(uint32_t index, const expr* key) {
    key = bind_map->whnf(key);

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
