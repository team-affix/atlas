#ifndef I_BIND_MAP_HPP
#define I_BIND_MAP_HPP

#include "../value_objects/expr.hpp"

struct i_bind_map {
    virtual ~i_bind_map() = default;
    virtual const expr* whnf(const expr*) = 0;
    virtual void push(const expr*, const expr*) = 0;
    virtual void process_step() = 0;
    virtual void clear() = 0;
};

#endif
