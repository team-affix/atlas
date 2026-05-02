#ifndef I_BIND_MAP_HPP
#define I_BIND_MAP_HPP

#include <queue>
#include "../value_objects/expr.hpp"

struct i_bind_map {
    virtual ~i_bind_map() = default;
    virtual const expr* whnf(const expr*) = 0;
    virtual bool unify(const expr*, const expr*, std::queue<uint32_t>&) = 0;
};

#endif
