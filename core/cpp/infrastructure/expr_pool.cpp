#include "infrastructure/expr_pool.hpp"

const expr* expr_pool::make_functor(uint32_t id, const std::vector<const expr*>& args) {
    return intern(expr{expr::functor{id, args}});
}

const expr* expr_pool::make_var(uint32_t i) {
    return intern(expr{expr::var{i}});
}

const expr* expr_pool::import(const expr* e) {
    if (std::holds_alternative<expr::var>(e->content))
        return intern(expr{e->content});
    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        std::vector<const expr*> imported_args;
        imported_args.reserve(f->args.size());
        for (const expr* arg : f->args)
            imported_args.push_back(import(arg));
        return make_functor(f->id, imported_args);
    }
    throw std::runtime_error("Unsupported expression type");
}

const expr* expr_pool::intern(expr&& e) {
    return &*exprs.emplace(std::move(e)).first;
}

size_t expr_pool::size() const {
    return exprs.size();
}
