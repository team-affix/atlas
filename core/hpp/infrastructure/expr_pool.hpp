#ifndef EXPR_POOL_HPP
#define EXPR_POOL_HPP

#include <set>
#include "interfaces/i_get_expr_count.hpp"
#include "interfaces/i_import_expr.hpp"
#include "interfaces/i_make_functor.hpp"
#include "interfaces/i_make_var.hpp"
#include "value_objects/expr.hpp"

struct expr_pool
    : i_make_functor
    , i_make_var
    , i_import_expr
    , i_get_expr_count {
    const expr* make(const std::string& name, const std::vector<const expr*>& args) override;
    const expr* make(uint32_t) override;
    const expr* import(const expr*) override;
    size_t size() const override;
private:
    const expr* intern(expr&&);

    std::set<expr> exprs;
};

#endif
