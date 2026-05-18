#ifndef I_SIM_TEARDOWN_HPP
#define I_SIM_TEARDOWN_HPP

struct i_sim_teardown {
    virtual ~i_sim_teardown() = default;
    virtual void teardown() = 0;
};

#endif
