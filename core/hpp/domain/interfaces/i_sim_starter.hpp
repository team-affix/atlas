#ifndef I_SIM_STARTER_HPP
#define I_SIM_STARTER_HPP

struct i_sim_starter {
    virtual ~i_sim_starter() = default;
    virtual void start() = 0;
};

#endif
