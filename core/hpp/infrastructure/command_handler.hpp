#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

template<typename Command>
struct command_handler {
    virtual ~command_handler() = default;
    virtual void operator()(const Command&) = 0;
};

#endif
