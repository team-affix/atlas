#ifndef I_MAKE_VAR_HPP
#define I_MAKE_VAR_HPP

#include <cstdint>
#include "value_objects/expr.hpp"

struct i_make_var {
    virtual ~i_make_var() = default;
    virtual const expr* make(uint32_t) = 0;
};

#endif

