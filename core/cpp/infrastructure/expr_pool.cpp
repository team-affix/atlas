#include <stdexcept>
#include "../../hpp/infrastructure/expr_pool.hpp"
#include "../../hpp/bootstrap/locator.hpp"
#include "../../hpp/utility/backtrackable_set_insert.hpp"

expr_pool::expr_pool() :
    exprs(locator::resolve<i_trail>(), {}) {
}

const expr* expr_pool::functor(const std::string& name, std::vector<const expr*> args) {
    return intern(expr{expr::functor{name, std::move(args)}});
}

const expr* expr_pool::var(uint32_t i) {
    return intern(expr{expr::var{i}});
}

const expr* expr_pool::import(const expr* e) {
    // if the expression is a var, just intern it
    if (std::holds_alternative<expr::var>(e->content))
        return intern(expr{e->content});

    // if the expression is a functor, recursively import all args then rebuild
    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        std::vector<const expr*> imported_args;
        imported_args.reserve(f->args.size());
        for (const expr* arg : f->args)
            imported_args.push_back(import(arg));
        return functor(f->name, std::move(imported_args));
    }

    throw std::runtime_error("Unsupported expression type");
}

const expr* expr_pool::intern(expr&& e) {
    // check if the expression is already in the pool
    auto original_it = exprs.get().find(e);
    if (original_it != exprs.get().end())
        return &*original_it;

    // if the expression is not in the pool, insert it
    auto insert_mut = std::make_unique<
        backtrackable_set_insert<
        std::set<expr>>>(std::move(e));
    exprs.mutate(std::move(insert_mut));
    
    // return the new expression
    auto it = exprs.get().find(e);
    return &*it;
}
