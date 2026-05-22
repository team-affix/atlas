#ifndef EXPR_POOL_HPP
#define EXPR_POOL_HPP

#include <set>
#include "../interfaces/i_expr_pool.hpp"
#include "../value_objects/expr.hpp"
#include "../utility/tracked.hpp"
#include "../utility/i_trail.hpp"

struct expr_pool : i_expr_pool {
    expr_pool(i_trail& t);
    const expr* functor(const std::string& name, std::vector<const expr*> args) override;
    const expr* var(uint32_t) override;
    const expr* import(const expr*) override;
    size_t size() const override;
private:
    const expr* intern(expr&&);

    tracked<std::set<expr>> exprs;
};

#endif
