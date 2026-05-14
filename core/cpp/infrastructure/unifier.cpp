#include "../../hpp/infrastructure/unifier.hpp"
#include "../../hpp/bootstrap/locator.hpp"
#include "../../hpp/domain/interfaces/i_factory.hpp"

unifier::unifier() :
    local_(locator::locate<i_factory<i_squash>>().make()),
    common_(locator::locate<i_squash>()) {
}

bool unifier::unify(const expr* lhs, const expr* rhs, i_queue<uint32_t>& var_set) {
        // WHNF the lhs and rhs
    lhs = whnf(lhs);
    rhs = whnf(rhs);

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
        local_->bind(young->index, target);
        var_set.push(young->index);
        return true;
    }

    // If one side is a variable, bind it to the other side
    const auto [v, other_e] = lv
        ? std::pair{lv, rhs}
        : std::pair{rv, lhs};
    if (v) {
        if (occurs_check(v->index, other_e))
            return false;
        local_->bind(v->index, other_e);
        var_set.push(v->index);
        return true;
    }

    // At this point, they are both functors
    const expr::functor& lf = std::get<expr::functor>(lhs->content);
    const expr::functor& rf = std::get<expr::functor>(rhs->content);
    if (lf.name != rf.name || lf.args.size() != rf.args.size())
        return false;
    for (size_t i = 0; i < lf.args.size(); ++i)
        if (!unify(lf.args[i], rf.args[i], var_set))
            return false;
    return true;
}

bool unifier::occurs_check(uint32_t index, const expr* key) {
    key = whnf(key);

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

const expr* unifier::whnf(const expr* e) {
    // if functor, then already in WHNF
    if (std::holds_alternative<expr::functor>(e->content))
        return e;

    const expr::var& v = std::get<expr::var>(e->content);
    
    // try local rep(), if it fails, try common rep()
    if(const expr* local_rep = local_->rep(v.index))
        return local_rep;
    
    // if common rep() fails, then just 
    if(const expr* common_rep = common_.rep(v.index))
        return common_rep;

    return e;
}