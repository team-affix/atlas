#ifndef I_COPIER_HPP
#define I_COPIER_HPP

#include "value_objects/expr.hpp"
#include "value_objects/translation_map.hpp"

struct i_copier {
    virtual ~i_copier() = default;
    virtual const expr* copy(const expr*, translation_map&) const = 0;
};

#endif
