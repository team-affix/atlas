#ifndef I_UNIFIER_HPP
#define I_UNIFIER_HPP

#include <cstdint>
#include <unordered_set>
#include "../value_objects/expr.hpp"

struct i_unifier {
    virtual ~i_unifier() = default;
    virtual bool unify(const expr*, const expr*, std::unordered_set<uint32_t>&) = 0;
};

#endif
