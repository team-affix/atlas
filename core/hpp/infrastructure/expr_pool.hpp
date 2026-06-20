#ifndef EXPR_POOL_HPP
#define EXPR_POOL_HPP

#include <set>
#include <stdexcept>
#include <vector>
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "value_objects/expr.hpp"

struct expr_pool : i_make_functor, i_make_var {
    const expr* make_functor(uint32_t id, const std::vector<const expr*>& args) override;
    const expr* make_var(uint32_t) override;
    const expr* import(const expr*);
    size_t size() const;
private:
    const expr* intern(expr&&);
    std::set<expr> exprs;
};

inline const expr* expr_pool::make_functor(uint32_t id, const std::vector<const expr*>& args) {
    return intern(expr{expr::functor{id, args}});
}

inline const expr* expr_pool::make_var(uint32_t i) {
    return intern(expr{expr::var{i}});
}

inline const expr* expr_pool::import(const expr* e) {
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

inline const expr* expr_pool::intern(expr&& e) {
    return &*exprs.emplace(std::move(e)).first;
}

inline size_t expr_pool::size() const {
    return exprs.size();
}

#endif
