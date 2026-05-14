#ifndef I_SQUASH_HPP
#define I_SQUASH_HPP

#include <cstdint>
#include "../value_objects/expr.hpp"

struct i_squash {
    virtual ~i_squash() = default;
    virtual void bind(uint32_t, const expr*) = 0;
    virtual const expr* rep(uint32_t) = 0;
};

#endif
