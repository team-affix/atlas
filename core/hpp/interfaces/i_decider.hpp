#ifndef I_DECIDER_HPP
#define I_DECIDER_HPP

struct i_decider {
    virtual ~i_decider() = default;
    virtual void decide() const = 0;
};

#endif
