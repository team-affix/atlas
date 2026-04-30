#ifndef I_TRACKED_HPP
#define I_TRACKED_HPP

#include <cstddef>
#include <functional>

template<typename T>
struct i_tracked {
    virtual ~i_tracked() = default;
    virtual void mutate(std::function<void(T&)>, std::function<void(T&)>) = 0;
    virtual const T& get() const = 0;
};

#endif
