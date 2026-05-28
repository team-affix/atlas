#ifndef I_CLEAR_BINDINGS_HPP
#define I_CLEAR_BINDINGS_HPP

struct i_clear_bindings {
    virtual ~i_clear_bindings() = default;
    virtual void clear_bindings() = 0;
};

#endif
