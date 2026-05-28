#ifndef I_GET_RESOLUTION_COUNT_HPP
#define I_GET_RESOLUTION_COUNT_HPP

#include <cstddef>

struct i_get_resolution_count {
    virtual ~i_get_resolution_count() = default;
    virtual size_t get_resolution_count() const = 0;
};

#endif

