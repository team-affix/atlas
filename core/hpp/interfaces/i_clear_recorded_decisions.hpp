#ifndef I_CLEAR_RECORDED_DECISIONS_HPP
#define I_CLEAR_RECORDED_DECISIONS_HPP

struct i_clear_recorded_decisions {
    virtual ~i_clear_recorded_decisions() = default;
    virtual void clear_recorded_decisions() = 0;
};

#endif
