#ifndef SCALAR_ADD_F64_HPP
#define SCALAR_ADD_F64_HPP

#include <compare>

struct scalar_add_f64 {
    double amount;
    auto operator<=>(const scalar_add_f64&) const = default;
};

#endif
