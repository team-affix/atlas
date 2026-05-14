#ifndef I_VAR_ID_EXTRACTOR_HPP
#define I_VAR_ID_EXTRACTOR_HPP

#include "i_visitor.hpp"
#include "../value_objects/expr.hpp"

struct i_var_id_extractor : i_visitor<const expr*> {
    virtual ~i_var_id_extractor() = default;
};

#endif
