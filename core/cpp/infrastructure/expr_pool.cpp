#include <stdexcept>
#include "../../../hpp/domain/data_structures/expr_pool.hpp"

expr_pool::expr_pool(trail& t) : trail_ref(t) {
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

size_t expr_pool::size() const {
    return exprs.size();
}

const expr* expr_pool::intern(expr&& e) {
    auto [it, inserted] = exprs.insert(std::move(e));
    if (inserted) trail_ref.log([this, it]() { exprs.erase(it); });
    return &*it;
}
