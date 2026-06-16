#ifndef I_NORMALIZER_HPP
#define I_NORMALIZER_HPP

#include "value_objects/expr.hpp"
#include "value_objects/framed_expr.hpp"

struct i_normalizer {
    virtual ~i_normalizer() = default;
    virtual const expr* normalize(framed_expr) = 0;
};

#endif
