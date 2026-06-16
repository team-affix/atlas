#ifndef I_GLOBALIZER_HPP
#define I_GLOBALIZER_HPP

#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

struct i_globalizer {
    virtual ~i_globalizer() = default;
    virtual const expr* globalize(framed_expr) = 0;
};

#endif
