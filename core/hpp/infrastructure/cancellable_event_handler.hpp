#ifndef CANCELLABLE_EVENT_HANDLER_HPP
#define CANCELLABLE_EVENT_HANDLER_HPP

#include "event_handler.hpp"
#include "cancellation_token.hpp"
#include "../bootstrap/resolver.hpp"

template<typename Event>
struct cancellable_event_handler : event_handler<Event> {
    virtual ~cancellable_event_handler() = default;
    cancellable_event_handler();
    virtual void execute() = 0;
#ifndef DEBUG
private:
#endif
    void operator()(const Event&) final override;
    const cancellation_token& token;
};

template<typename Event>
cancellable_event_handler<Event>::cancellable_event_handler() :
    event_handler<Event>(),
    token(resolver::resolve<cancellation_token>()) {
}

template<typename Event>
void cancellable_event_handler<Event>::operator()(const Event& event) {
    if (token.is_cancelled())
        return;
    execute();
}

#endif
