#include "../hpp/bind_map.hpp"

bind_map::bind_map(trail& trail_ref) : trail_ref(trail_ref) {

}

const expr* bind_map::whnf(const expr* key) {
    // If the key is not a variable, it is already in WHNF
    if (!std::holds_alternative<expr::var>(key->content))
        return key;

    // Get the variable out of the key
    const expr::var& var = std::get<expr::var>(key->content);

    // Check if the variable is bound
    auto it = bindings.find(var.index);
    
    // If the variable is not bound, return the key
    if (it == bindings.end())
        return key;

    // Get the bound value
    const expr* bound_value = it->second;
        
    // WHNF the bound value
    const expr* whnf_bound_value = whnf(bound_value);

    // Collapse the binding
    bind(var.index, whnf_bound_value);

    return whnf_bound_value;
}

bool bind_map::unify(const expr* lhs, const expr* rhs) {
    // WHNF the lhs and rhs
    lhs = whnf(lhs);
    rhs = whnf(rhs);

    // get the lhs and rhs var handles if they are variables
    const expr::var* lv = std::get_if<expr::var>(&lhs->content);
    const expr::var* rv = std::get_if<expr::var>(&rhs->content);

    // if they are the same variable, unification succeeds trivially
    if (lv && rv && lv->index == rv->index)
        return true;
    
    // If the lhs is a variable, add a binding to the whnf of the rhs
    if (lv) {
        if (occurs_check(lv->index, rhs))
            return false;
        bind(lv->index, rhs);
        return true;
    }

    // If the rhs is a variable, add a binding to the whnf of the lhs
    if (rv) {
        if (occurs_check(rv->index, lhs))
            return false;
        bind(rv->index, lhs);
        return true;
    }

    // If they are not the same type, unification fails
    if (lhs->content.index() != rhs->content.index())
        return false;

    // If they are both functors, unify name, arity, and all args
    if (std::holds_alternative<expr::functor>(lhs->content)) {
        const expr::functor& lf = std::get<expr::functor>(lhs->content);
        const expr::functor& rf = std::get<expr::functor>(rhs->content);
        if (lf.name != rf.name || lf.args.size() != rf.args.size())
            return false;
        for (size_t i = 0; i < lf.args.size(); ++i)
            if (!unify(lf.args[i], rf.args[i]))
                return false;
        return true;
    }

    return false;

}

bool bind_map::occurs_check(uint32_t index, const expr* key) {
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

void bind_map::bind(uint32_t index, const expr* value) {
    // look up the entry for the index
    auto it = bindings.find(index);

    if (it == bindings.end()) {
        // if the value is not found, insert it
        trail_ref.log([this, index]{bindings.erase(index);});
        it = bindings.insert({index, value}).first;
    }
    else {
        // Get the old value
        const expr* old_value = it->second;
        
        // If the new value is the same as the old value, do nothing
        if (old_value == value)
            return;

        // If the new value is different from the old value, insert it
        trail_ref.log([it, old_value]{it->second = old_value;});

        // Update the value
        it->second = value;
    }
    
    return;
}
