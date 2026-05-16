#ifndef I_EXPR_PRINTER_HPP
#define I_EXPR_PRINTER_HPP

#include "../value_objects/expr.hpp"

struct i_expr_printer {
    virtual ~i_expr_printer() = default;
    virtual void print(const expr*) const = 0;
};

#endif
