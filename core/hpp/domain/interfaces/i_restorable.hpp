#ifndef I_RESTORABLE_HPP
#define I_RESTORABLE_HPP

struct i_restorable {
    virtual ~i_restorable() = default;
    virtual void restore() = 0;
};

#endif
