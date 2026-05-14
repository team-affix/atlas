#ifndef I_BIND_MAP_HPP
#define I_BIND_MAP_HPP

#include <cstdint>
#include "../value_objects/expr.hpp"

struct i_bind_map {
    virtual ~i_bind_map() = default;
    virtual void bind(uint32_t, const expr*) = 0;
    virtual const expr* whnf(const expr*) = 0;
};

#endif
