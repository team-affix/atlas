#ifndef I_IMPORT_EXPR_HPP
#define I_IMPORT_EXPR_HPP

#include "../value_objects/expr.hpp"

struct i_import_expr {
    virtual ~i_import_expr() = default;
    virtual const expr* import(const expr*) = 0;
};

#endif

