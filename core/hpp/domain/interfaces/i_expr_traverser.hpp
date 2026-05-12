#ifndef I_EXPR_TRAVERSER_HPP
#define I_EXPR_TRAVERSER_HPP

#include "i_acceptor.hpp"
#include "../value_objects/expr.hpp"

struct i_expr_traverser : i_acceptor<const expr*> {
    virtual ~i_expr_traverser() = default;
};

#endif
