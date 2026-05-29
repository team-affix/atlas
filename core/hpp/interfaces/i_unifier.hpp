#ifndef I_UNIFIER_HPP
#define I_UNIFIER_HPP

#include <cstdint>
#include "value_objects/expr.hpp"
#include "infrastructure/coroutine.hpp"

struct i_unifier {
    virtual ~i_unifier() = default;
    virtual coroutine<uint32_t, bool> unify(const expr*, const expr*) = 0;
};

#endif
