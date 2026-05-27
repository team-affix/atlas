#ifndef I_ACTIVE_GOALS_SIZE_HPP
#define I_ACTIVE_GOALS_SIZE_HPP

#include <cstddef>

struct i_active_goals_size {
    virtual ~i_active_goals_size() = default;
    virtual size_t active_goals_size() const = 0;
};

#endif
