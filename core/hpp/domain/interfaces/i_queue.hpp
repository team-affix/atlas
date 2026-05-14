#ifndef I_QUEUE_HPP
#define I_QUEUE_HPP

#include <cstddef>

template<typename T>
struct i_queue {
    virtual ~i_queue() = default;
    virtual void push(T) = 0;
    virtual T pop() = 0;
    virtual T& front() = 0;
    virtual const T& front() const = 0;
    virtual void clear() = 0;
    virtual bool empty() const = 0;
    virtual size_t size() const = 0;
};

#endif
