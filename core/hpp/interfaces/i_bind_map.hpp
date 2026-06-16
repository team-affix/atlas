#ifndef I_BIND_MAP_HPP
#define I_BIND_MAP_HPP

#include <cstdint>
#include "value_objects/framed_expr.hpp"

struct i_bind_map {
    virtual ~i_bind_map() = default;
    virtual void bind(uint32_t global_key, framed_expr value) = 0;
    virtual framed_expr whnf(framed_expr) = 0;
};

#endif
