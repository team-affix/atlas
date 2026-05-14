#ifndef I_UNIFIER_HPP
#define I_UNIFIER_HPP

#include "../value_objects/expr.hpp"
#include "i_rep_change_sink.hpp"

struct i_unifier {
    virtual ~i_unifier() = default;
    virtual bool unify(const expr*, const expr*, i_rep_change_sink&) = 0;
};

#endif
