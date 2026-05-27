#ifndef I_CHECK_ACTIVE_GOALS_EMPTY_HPP
#define I_CHECK_ACTIVE_GOALS_EMPTY_HPP

struct i_check_active_goals_empty {
    virtual ~i_check_active_goals_empty() = default;
    virtual bool empty() const = 0;
};

#endif
