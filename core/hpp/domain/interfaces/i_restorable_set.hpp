#ifndef I_RESTORABLE_SET_HPP
#define I_RESTORABLE_SET_HPP

#include "i_restorable.hpp"

template<typename S>
struct i_restorable_set : i_restorable {
    virtual ~i_restorable_set() = default;
    virtual void restore() = 0;
    virtual void insert(const S::value_type&) = 0;
    virtual void erase(const S::value_type&) = 0;
    virtual const S& get() const = 0;
};

#endif
