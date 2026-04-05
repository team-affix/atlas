#include "../hpp/bind_map.hpp"

bind_map::bind_map(trail& trail_ref, registry<expr>& er) :
    trail_ref(trail_ref), er(er) {

}

expr_id bind_map::whnf(expr_id key) {
    // Get the expression from the registry
    const expr* e = er.deref(key);
    
    // If the key is not a variable, it is already in WHNF
    if (!std::holds_alternative<expr::var>(e->content))
        return key;

    // Get the variable out of the key
    const expr::var& var = std::get<expr::var>(e->content);

    // Check if the variable is bound
    auto it = bindings.find(var.index);
    
    // If the variable is not bound, return the key
    if (it == bindings.end())
        return key;

    // Get the bound value
    expr_id bound_value = it->second;
        
    // WHNF the bound value
    expr_id whnf_bound_value = whnf(bound_value);

    // Collapse the binding
    bind(var.index, whnf_bound_value);

    return whnf_bound_value;
}

bool bind_map::unify(expr_id lhs, expr_id rhs) {
    // WHNF the lhs and rhs
    lhs = whnf(lhs);
    rhs = whnf(rhs);

    // Get the lhs and rhs expressions from the registry
    const expr* lhs_e = er.deref(lhs);
    const expr* rhs_e = er.deref(rhs);

    // get the lhs and rhs var handles if they are variables
    const expr::var* lv = std::get_if<expr::var>(&lhs_e->content);
    const expr::var* rv = std::get_if<expr::var>(&rhs_e->content);

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
    if (lhs_e->content.index() != rhs_e->content.index())
        return false;

    // If they are both atoms, unify the values
    if (std::holds_alternative<expr::atom>(lhs_e->content)) {
        const expr::atom& lAtom = std::get<expr::atom>(lhs_e->content);
        const expr::atom& rAtom = std::get<expr::atom>(rhs_e->content);
        return lAtom.value == rAtom.value;
    }

    // If they are both cons cells, unify the children
    if (std::holds_alternative<expr::cons>(lhs_e->content)) {
        const expr::cons& lCons = std::get<expr::cons>(lhs_e->content);
        const expr::cons& rCons = std::get<expr::cons>(rhs_e->content);
        return unify(lCons.lhs, rCons.lhs) && unify(lCons.rhs, rCons.rhs);
    }

    return false;

}

bool bind_map::occurs_check(uint32_t index, expr_id key) {
    key = whnf(key);

    const expr* key_e = er.deref(key);

    if (const expr::var* var = std::get_if<expr::var>(&key_e->content))
        return var->index == index;

    if (const expr::cons* cons = std::get_if<expr::cons>(&key_e->content)) {
        return occurs_check(index, cons->lhs) || occurs_check(index, cons->rhs);
    }

    return false;
}

void bind_map::bind(uint32_t index, expr_id value) {
    // look up the entry for the index
    auto it = bindings.find(index);

    if (it == bindings.end()) {
        // if the value is not found, insert it
        trail_ref.log(
            [this, index]{bindings.erase(index);},
            [this, index, value]{bindings.insert({index, value});}
        );
        it = bindings.insert({index, value}).first;
    }
    else {
        // Get the old value
        expr_id old_value = it->second;
        
        // If the new value is the same as the old value, do nothing
        if (old_value == value)
            return;

        // If the new value is different from the old value, insert it
        trail_ref.log(
            [this, index, old_value]{bindings[index] = old_value;},
            [this, index, value]{bindings[index] = value;}
        );

        // Update the value
        it->second = value;
    }
    
    return;
}
