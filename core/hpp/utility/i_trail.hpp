#ifndef I_TRAIL_HPP
#define I_TRAIL_HPP

#include <memory>
#include "i_backtrackable.hpp"

struct i_trail {
    virtual ~i_trail() = default;
    virtual void push() = 0;
    virtual void pop() = 0;
    virtual void log(std::unique_ptr<i_backtrackable>) = 0;
};

#endif
