#ifndef I_DATABASE_HPP
#define I_DATABASE_HPP

#include "../value_objects/rule.hpp"

struct i_database {
    virtual ~i_database() = default;
    virtual const rule& at(size_t) const = 0;
    virtual size_t size() const = 0;
};

#endif
