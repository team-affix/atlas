#ifndef I_ACTIVATE_INITIAL_GOALS_AND_CANDIDATES_HPP
#define I_ACTIVATE_INITIAL_GOALS_AND_CANDIDATES_HPP

struct i_activate_initial_goals_and_candidates {
    virtual ~i_activate_initial_goals_and_candidates() = default;
    virtual bool activate_initial_goals_and_candidates() = 0;
};

#endif
