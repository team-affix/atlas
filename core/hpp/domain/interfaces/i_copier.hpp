#ifndef I_COPIER_HPP
#define I_COPIER_HPP

#include "../value_objects/expr.hpp"
#include "i_translation_map.hpp"

struct i_copier {
    virtual ~i_copier() = default;
    virtual const expr* copy(const expr*, i_translation_map&) = 0;
};

#endif
