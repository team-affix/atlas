#ifndef I_GET_GROUNDED_WEIGHT_HPP
#define I_GET_GROUNDED_WEIGHT_HPP

struct i_get_grounded_weight {
    virtual ~i_get_grounded_weight() = default;
    virtual double get() const = 0;
};

#endif
