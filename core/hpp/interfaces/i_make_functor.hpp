#ifndef I_MAKE_FUNCTOR_HPP
#define I_MAKE_FUNCTOR_HPP

#include <string>
#include <vector>
#include "../value_objects/expr.hpp"

struct i_make_functor {
    virtual ~i_make_functor() = default;
    virtual const expr* make(const std::string& name, const std::vector<const expr*>& args) = 0;
};

#endif

