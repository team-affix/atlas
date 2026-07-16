#include "infrastructure/cumulative_grounded_weight.hpp"

cumulative_grounded_weight::cumulative_grounded_weight() : value_(0.0) {}


void cumulative_grounded_weight::accumulate(double w) {
    value_ += w;
}

double cumulative_grounded_weight::get() const {
    return value_;
}

void cumulative_grounded_weight::clear() {
    value_ = 0.0;
}