#ifndef I_CHECK_UNIT_GOALS_EMPTY_HPP
#define I_CHECK_UNIT_GOALS_EMPTY_HPP

struct i_check_unit_goals_empty {
    virtual ~i_check_unit_goals_empty() = default;
    virtual bool empty() const = 0;
};

#endif
