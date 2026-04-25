#include "../hpp/weight_expander.hpp"

weight_expander::weight_expander(const double& weight, const rule& r, double& cgw) : cgw(cgw) {
    if (r.body.size() == 0) {
        cgw += weight;
        return;
    }

    child_weight = weight / (double)r.body.size();
}

double weight_expander::operator()() {
    return child_weight;
}
