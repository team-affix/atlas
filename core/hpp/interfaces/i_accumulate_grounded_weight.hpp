#ifndef I_ACCUMULATE_GROUNDED_WEIGHT_HPP
#define I_ACCUMULATE_GROUNDED_WEIGHT_HPP

struct i_accumulate_grounded_weight {
    virtual ~i_accumulate_grounded_weight() = default;
    virtual void accumulate(double) = 0;
};

#endif
