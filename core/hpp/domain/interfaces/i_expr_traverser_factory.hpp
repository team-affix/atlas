#ifndef I_EXPR_TRAVERSER_FACTORY_HPP
#define I_EXPR_TRAVERSER_FACTORY_HPP

#include "i_factory.hpp"
#include "i_expr_traverser.hpp"
#include "../value_objects/expr.hpp"

struct i_expr_traverser_factory : i_factory<i_expr_traverser, const expr*> {
    virtual ~i_expr_traverser_factory() = default;
};

#endif
