#ifndef I_RANDOM_ACCESS_HPP
#define I_RANDOM_ACCESS_HPP

#include <cstddef>

template<typename T>
struct i_random_access {
    virtual ~i_random_access() = default;
    virtual T select(size_t index) const = 0;
};

#endif
