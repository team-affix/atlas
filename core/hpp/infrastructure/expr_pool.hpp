#ifndef EXPR_POOL_HPP
#define EXPR_POOL_HPP

#include <set>
#include "../domain/interfaces/i_expr_pool.hpp"
#include "../domain/value_objects/expr.hpp"
#include "../utility/tracked.hpp"

struct expr_pool : i_expr_pool {
    expr_pool();
    const expr* functor(const std::string& name, std::vector<const expr*> args) override;
    const expr* var(uint32_t) override;
    const expr* import(const expr*) override;
#ifndef DEBUG
private:
#endif
    const expr* intern(expr&&);

    tracked<std::set<expr>> exprs;
};

#endif
