#include "infrastructure/remaining_work.hpp"

remaining_work::remaining_work() : value_(0.0) {}

void remaining_work::add(double w) {
    value_ += w;
}

void remaining_work::subtract(double w) {
    value_ -= w;
}

double remaining_work::get() const {
    return value_;
}

void remaining_work::clear() {
    value_ = 0.0;
}
