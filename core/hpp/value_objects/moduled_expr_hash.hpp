#ifndef MODULED_EXPR_HASH_HPP
#define MODULED_EXPR_HASH_HPP

#include <cstddef>
#include "value_objects/moduled_expr.hpp"

struct moduled_expr_hash {
    size_t operator()(const moduled_expr& term) const noexcept;
};

#endif
