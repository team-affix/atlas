#ifndef I_REP_CHANGE_SINK_HPP
#define I_REP_CHANGE_SINK_HPP

#include <cstdint>

struct i_rep_change_sink {
    virtual ~i_rep_change_sink() = default;
    virtual void push(uint32_t) = 0;
    virtual uint32_t pop() = 0;
    virtual bool empty() const = 0;
};

#endif
