#ifndef I_SIM_STOPPER_HPP
#define I_SIM_STOPPER_HPP

struct i_sim_stopper {
    virtual ~i_sim_stopper() = default;
    virtual void init_stop() = 0;
    virtual void finish_stop() = 0;
};

#endif
