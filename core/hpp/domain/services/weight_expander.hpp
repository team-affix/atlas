#ifndef WEIGHT_EXPANDER_HPP
#define WEIGHT_EXPANDER_HPP

#include "../value_objects/rule.hpp"

struct weight_expander {
    weight_expander(const double& weight, const rule& r, double& cgw);
    double operator()();
#ifndef DEBUG
private:
#endif
    double& cgw;
    
    double child_weight;
};

#endif
