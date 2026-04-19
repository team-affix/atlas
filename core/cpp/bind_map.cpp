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

    // If they are both atoms, unify the values
    if (std::holds_alternative<expr::atom>(lhs->content)) {
        const expr::atom& lAtom = std::get<expr::atom>(lhs->content);
        const expr::atom& rAtom = std::get<expr::atom>(rhs->content);
        return lAtom.value == rAtom.value;
    }

    // If they are both cons cells, unify the children
    if (std::holds_alternative<expr::cons>(lhs->content)) {
        const expr::cons& lCons = std::get<expr::cons>(lhs->content);
        const expr::cons& rCons = std::get<expr::cons>(rhs->content);
        return unify(lCons.lhs, rCons.lhs) && unify(lCons.rhs, rCons.rhs);
    }

    // If they are both preds, names and arities must match, then unify args
    if (std::holds_alternative<expr::pred>(lhs->content)) {
        const expr::pred& lPred = std::get<expr::pred>(lhs->content);
        const expr::pred& rPred = std::get<expr::pred>(rhs->content);
        if (lPred.name != rPred.name || lPred.args.size() != rPred.args.size())
            return false;
        for (size_t i = 0; i < lPred.args.size(); ++i)
            if (!unify(lPred.args[i], rPred.args[i]))
                return false;
        return true;
    }

    return false;

}

bool bind_map::occurs_check(uint32_t index, const expr* key) {
    key = whnf(key);

    if (const expr::var* var = std::get_if<expr::var>(&key->content))
        return var->index == index;

    if (const expr::cons* cons = std::get_if<expr::cons>(&key->content))
        return occurs_check(index, cons->lhs) || occurs_check(index, cons->rhs);

    if (const expr::pred* p = std::get_if<expr::pred>(&key->content)) {
        for (const expr* arg : p->args)
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
