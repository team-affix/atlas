#ifndef I_UNIFIER_FACTORY_HPP
#define I_UNIFIER_FACTORY_HPP

#include "i_factory.hpp"
#include "i_bind_map.hpp"
#include "i_unifier.hpp"

struct i_unifier_factory : i_factory<i_unifier, i_bind_map&> {
    virtual ~i_unifier_factory() = default;
};

#endif
