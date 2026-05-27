#ifndef I_SET_UP_SIM_HPP
#define I_SET_UP_SIM_HPP

struct i_set_up_sim {
    virtual ~i_set_up_sim() = default;
    virtual void set_up() = 0;
};

#endif
