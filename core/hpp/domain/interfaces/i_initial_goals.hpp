#ifndef I_INITIAL_GOALS_HPP
#define I_INITIAL_GOALS_HPP

#include <cstddef>
#include "../value_objects/expr.hpp"

struct i_initial_goals {
    virtual ~i_initial_goals() = default;
    virtual const expr* at(size_t) const = 0;
    virtual size_t size() const = 0;
};

#endif
