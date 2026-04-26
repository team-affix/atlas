#ifndef EVENT_HANDLER_HPP
#define EVENT_HANDLER_HPP

template<typename Event>
struct event_handler {
    virtual ~event_handler() = default;
    virtual void operator()(const Event&) = 0;
};

#endif
