#ifndef I_VAR_EXTRACTOR_HPP
#define I_VAR_EXTRACTOR_HPP

#include "i_visitor.hpp"
#include "../value_objects/expr.hpp"

struct i_var_extractor : i_visitor<const expr*> {
    virtual ~i_var_extractor() = default;
};

#endif
