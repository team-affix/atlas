#include <stdexcept>
#include "../hpp/copier.hpp"

copier::copier(sequencer& sequencer_ref, expr_pool& expr_pool_ref)
    : sequencer_ref(sequencer_ref), expr_pool_ref(expr_pool_ref) {

}

const expr* copier::operator()(const expr* e, std::map<uint32_t, uint32_t>& variable_map) {
    // If the expression is an atom, return the atom unchanged
    if (std::holds_alternative<expr::atom>(e->content))
        return e;

    // If the expression is a variable
    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        // See if the variable has already been copied
        auto it = variable_map.find(v->index);
        
        // If the variable has does not have a mapping, create one
        if (it == variable_map.end())
            it = variable_map.insert({v->index, sequencer_ref()}).first;

        // Return the mapped variable
        return expr_pool_ref.var(it->second);
    }

    // If the expression is a cons cell, copy the car and cdr
    if (const expr::cons* c = std::get_if<expr::cons>(&e->content)) {
        const expr* copied_lhs = operator()(c->lhs, variable_map);
        const expr* copied_rhs = operator()(c->rhs, variable_map);
        return expr_pool_ref.cons(copied_lhs, copied_rhs);
    }

    // If the expression is a pred, copy each argument
    if (const expr::pred* p = std::get_if<expr::pred>(&e->content)) {
        std::vector<const expr*> copied_args;
        copied_args.reserve(p->args.size());
        for (const expr* arg : p->args)
            copied_args.push_back(operator()(arg, variable_map));
        return expr_pool_ref.pred(p->name, std::move(copied_args));
    }

    throw std::runtime_error("Unsupported expression type");
}

const expr::pred* copier::operator()(const expr::pred* p, std::map<uint32_t, uint32_t>& variable_map) {
    std::vector<const expr*> copied_args;
    copied_args.reserve(p->args.size());
    for (const expr* arg : p->args)
        copied_args.push_back(operator()(arg, variable_map));
    return &std::get<expr::pred>(expr_pool_ref.pred(p->name, std::move(copied_args))->content);
}