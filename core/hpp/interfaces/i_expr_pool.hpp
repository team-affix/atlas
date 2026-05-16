#ifndef I_EXPR_POOL_HPP
#define I_EXPR_POOL_HPP

#include <string>
#include "../value_objects/expr.hpp"

struct i_expr_pool {
    virtual ~i_expr_pool() = default;
    virtual const expr* functor(const std::string& name, std::vector<const expr*> args = {}) = 0;
    virtual const expr* var(uint32_t) = 0;
    virtual const expr* import(const expr*) = 0;
};

#endif