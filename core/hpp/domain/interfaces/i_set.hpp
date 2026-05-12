#ifndef I_SET_HPP
#define I_SET_HPP

#include <cstddef>

template<typename T>
struct i_set {
    virtual ~i_set() = default;
    virtual void insert(T) = 0;
    virtual bool contains(T) const = 0;
    virtual void erase(T) = 0;
    virtual void clear() = 0;
    virtual bool empty() const = 0;
    virtual size_t size() const = 0;
};

#endif
