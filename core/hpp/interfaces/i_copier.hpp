#ifndef I_COPIER_HPP
#define I_COPIER_HPP

#include <cstdint>
#include <unordered_map>
#include "../value_objects/expr.hpp"

struct i_copier {
    virtual ~i_copier() = default;
    virtual const expr* copy(const expr*, std::unordered_map<uint32_t, uint32_t>&) const = 0;
};

#endif
