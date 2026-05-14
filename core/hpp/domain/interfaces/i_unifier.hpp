#ifndef I_UNIFIER_HPP
#define I_UNIFIER_HPP

#include <memory>
#include "../value_objects/expr.hpp"
#include "i_rep_change_sink.hpp"
#include "i_bind_map.hpp"

struct i_unifier {
    virtual ~i_unifier() = default;
    i_unifier(std::unique_ptr<i_bind_map>);
    virtual bool unify(const expr*, const expr*, i_rep_change_sink&) = 0;

    std::unique_ptr<i_bind_map> bind_map;
};

#endif
