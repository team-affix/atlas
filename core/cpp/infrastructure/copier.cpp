#include <stdexcept>
#include "../../hpp/infrastructure/copier.hpp"
#include "../../hpp/bootstrap/locator.hpp"

copier::copier() :
    var_seq_ref(locator::locate<i_var_sequencer>()),
    expr_pool_ref(locator::locate<i_expr_pool>()) {
}

const expr* copier::copy(const expr* e, std::unordered_map<uint32_t, uint32_t>& variable_map) const {
    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        if (!variable_map.contains(v->index))
            variable_map.emplace(v->index, var_seq_ref.next());
        return expr_pool_ref.var(variable_map.at(v->index));
    }

    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        std::vector<const expr*> copied_args;
        copied_args.reserve(f->args.size());
        for (const expr* arg : f->args)
            copied_args.push_back(copy(arg, variable_map));
        return expr_pool_ref.functor(f->name, std::move(copied_args));
    }

    throw std::runtime_error("Unsupported expression type");
}
