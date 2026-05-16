#ifndef I_SIM_HPP
#define I_SIM_HPP

struct i_sim {
    virtual ~i_sim() = default;
    virtual bool run() = 0;
};

#endif
