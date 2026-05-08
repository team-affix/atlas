#ifndef I_SIM_RESTARTER_HPP
#define I_SIM_RESTARTER_HPP

struct i_sim_restarter {
    virtual ~i_sim_restarter() = default;
    virtual void begin_restart() = 0;
    virtual void complete_restart() = 0;
};

#endif
