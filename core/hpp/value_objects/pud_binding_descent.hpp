#ifndef PUD_BINDING_DESCENT_HPP
#define PUD_BINDING_DESCENT_HPP

#include "immer/map.hpp"
#include "value_objects/framed_expr.hpp"

struct pud_binding_descent {
    immer::map<uint32_t, framed_expr> bindings;
};

#endif
