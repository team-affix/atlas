#ifndef I_UNIFIER_HPP
#define I_UNIFIER_HPP

#include "../value_objects/expr.hpp"
#include "i_queue.hpp"

struct i_unifier {
    virtual ~i_unifier() = default;
    virtual bool unify(const expr*, const expr*, i_queue<uint32_t>&) = 0;
    virtual const expr* whnf(const expr*) = 0;
};

#endif
