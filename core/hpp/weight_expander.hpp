#ifndef WEIGHT_EXPANDER_HPP
#define WEIGHT_EXPANDER_HPP

#include "rule.hpp"

struct weight_expander {
    weight_expander(const double& weight, const rule& r);
    double operator()();
#ifndef DEBUG
private:
#endif
    double child_weight;
};

#endif
