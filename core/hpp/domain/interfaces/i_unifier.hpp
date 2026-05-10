#ifndef I_UNIFIER_HPP
#define I_UNIFIER_HPP

#include "../value_objects/expr.hpp"

struct i_unifier {
    virtual ~i_unifier() = default;
    virtual const expr* whnf(const expr*) = 0;
    virtual void push(const expr*, const expr*) = 0;
    virtual void process_step() = 0;
    virtual void clear() = 0;
};

#endif
