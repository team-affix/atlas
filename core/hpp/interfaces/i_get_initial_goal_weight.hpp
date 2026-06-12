#ifndef I_GET_INITIAL_GOAL_WEIGHT_HPP
#define I_GET_INITIAL_GOAL_WEIGHT_HPP

struct i_get_initial_goal_weight {
    virtual ~i_get_initial_goal_weight() = default;
    virtual double get() const = 0;
};

#endif
