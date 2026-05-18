#ifndef I_SIM_SETUP_HPP
#define I_SIM_SETUP_HPP

struct i_sim_setup {
    virtual ~i_sim_setup() = default;
    virtual void setup() = 0;
};

#endif
