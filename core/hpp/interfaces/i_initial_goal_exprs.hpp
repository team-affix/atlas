#ifndef I_INITIAL_GOAL_EXPRS_HPP
#define I_INITIAL_GOAL_EXPRS_HPP

#include <cstddef>
#include "../value_objects/expr.hpp"

struct i_initial_goal_exprs {
    virtual ~i_initial_goal_exprs() = default;
    virtual const expr* at(size_t) const = 0;
    virtual size_t size() const = 0;
};

#endif
