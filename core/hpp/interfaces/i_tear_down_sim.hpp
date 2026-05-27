#ifndef I_TEAR_DOWN_SIM_HPP
#define I_TEAR_DOWN_SIM_HPP

struct i_tear_down_sim {
    virtual ~i_tear_down_sim() = default;
    virtual void tear_down() = 0;
};

#endif
