#ifndef I_CLEAR_ACTIVE_GOALS_HPP
#define I_CLEAR_ACTIVE_GOALS_HPP

struct i_clear_active_goals {
    virtual ~i_clear_active_goals() = default;
    virtual void clear_active_goals() = 0;
};

#endif
