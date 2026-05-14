#ifndef I_REP_CHANGE_SINK_HPP
#define I_REP_CHANGE_SINK_HPP

#include <cstdint>
#include "i_acceptor.hpp"

struct i_rep_change_sink : i_acceptor<uint32_t> {
    virtual ~i_rep_change_sink() = default;
    virtual void push(uint32_t) = 0;
};

#endif
