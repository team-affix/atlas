#ifndef UBS_CLASS_HPP
#define UBS_CLASS_HPP

#include <compare>
#include <cstdint>
#include <optional>
#include "value_objects/framed_expr.hpp"

// One union-find class of variables: its root, how many variables it holds, its least variable
// (the representative) and the value the class is bound to.
struct ubs_class {
    uint32_t root;
    uint32_t size;
    uint32_t rep;
    std::optional<framed_expr> value;
    auto operator<=>(const ubs_class&) const = default;
};

#endif
