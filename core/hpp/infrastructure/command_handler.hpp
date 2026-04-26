#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include "event_handler.hpp"

template<typename Command>
struct command_handler {
    virtual ~command_handler() = default;
    virtual void operator()(const Command&) = 0;
};

#endif
