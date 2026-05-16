#ifndef I_NORMALIZER_HPP
#define I_NORMALIZER_HPP

#include "../value_objects/expr.hpp"

struct i_normalizer {
    virtual ~i_normalizer() = default;
    virtual const expr* normalize(const expr*) = 0;
};

#endif
