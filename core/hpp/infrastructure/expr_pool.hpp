#ifndef EXPR_POOL_HPP
#define EXPR_POOL_HPP

#include <set>
#include <stdexcept>
#include <vector>
#include "value_objects/expr.hpp"

struct expr_pool {
    const expr* make_functor(uint32_t id, const std::vector<const expr*>& args);
    const expr* make_var(uint32_t);
    const expr* import(const expr*);
    size_t size() const;
private:
    const expr* intern(expr&&);
    std::set<expr> exprs;
};

#endif
