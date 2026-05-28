#ifndef I_GET_INITIAL_GOAL_COUNT_HPP
#define I_GET_INITIAL_GOAL_COUNT_HPP

#include <cstddef>

struct i_get_initial_goal_count {
    virtual ~i_get_initial_goal_count() = default;
    virtual size_t count() const = 0;
};

#endif
