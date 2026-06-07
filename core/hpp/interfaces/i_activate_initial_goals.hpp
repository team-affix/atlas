#ifndef I_ACTIVATE_INITIAL_GOALS_HPP
#define I_ACTIVATE_INITIAL_GOALS_HPP

struct i_activate_initial_goals {
    virtual ~i_activate_initial_goals() = default;
    virtual bool activate_initial_goals() = 0;
};

#endif
