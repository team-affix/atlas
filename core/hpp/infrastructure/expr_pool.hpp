#ifndef EXPR_POOL_HPP
#define EXPR_POOL_HPP

#include <set>
#include <string>
#include "../value_objects/expr.hpp"
#include "../../infrastructure/trail.hpp"

struct expr_pool {
    expr_pool(trail&);
    const expr* functor(const std::string& name, std::vector<const expr*> args = {});
    const expr* var(uint32_t);
    const expr* import(const expr*);
    size_t size() const;
#ifndef DEBUG
private:
#endif
    const expr* intern(expr&&);
    trail& trail_ref;
    std::set<expr> exprs;
};

#endif
