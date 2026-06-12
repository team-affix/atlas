#ifndef CUMULATIVE_GROUNDED_WEIGHT_HPP
#define CUMULATIVE_GROUNDED_WEIGHT_HPP

#include "interfaces/i_accumulate_grounded_weight.hpp"
#include "interfaces/i_get_grounded_weight.hpp"
#include "interfaces/i_clear_grounded_weight.hpp"

struct cumulative_grounded_weight
    : i_accumulate_grounded_weight
    , i_get_grounded_weight
    , i_clear_grounded_weight {
    void accumulate(double w) override;
    double get() const override;
    void clear() override;
private:
    double value_{0.0};
};

#endif
