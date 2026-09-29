#ifndef VAR_COUNTER_HPP
#define VAR_COUNTER_HPP

#include "value_objects/expr.hpp"

struct var_counter {
    uint32_t count(const expr* e);
};

#endif
