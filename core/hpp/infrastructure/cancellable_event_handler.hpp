#ifndef CANCELLABLE_EVENT_HANDLER_HPP
#define CANCELLABLE_EVENT_HANDLER_HPP

#include "event_handler.hpp"

template<typename Event, typename CancellationEvent, typename ResetCancellationEvent>
struct cancellable_event_handler :
        event_handler<Event>,
        event_handler<CancellationEvent>,
        event_handler<ResetCancellationEvent> {
    virtual ~cancellable_event_handler() = default;
    cancellable_event_handler();
    virtual void execute(const Event&) = 0;
private:
    void handle(const Event&) final override;
    void handle(const CancellationEvent&) final override;
    void handle(const ResetCancellationEvent&) final override;
    bool cancelled;
};

template<typename Event, typename CancellationEvent, typename ResetCancellationEvent>
cancellable_event_handler<Event, CancellationEvent, ResetCancellationEvent>::cancellable_event_handler() :
    event_handler<Event>(),
    event_handler<CancellationEvent>(),
    event_handler<ResetCancellationEvent>(),
    cancelled(false) {
}

template<typename Event, typename CancellationEvent, typename ResetCancellationEvent>
void cancellable_event_handler<Event, CancellationEvent, ResetCancellationEvent>::handle(const Event& event) {
    if (cancelled)
        return;
    execute(event);
}

template<typename Event, typename CancellationEvent, typename ResetCancellationEvent>
void cancellable_event_handler<Event, CancellationEvent, ResetCancellationEvent>::handle(const CancellationEvent&) {
    cancelled = true;
}

template<typename Event, typename CancellationEvent, typename ResetCancellationEvent>
void cancellable_event_handler<Event, CancellationEvent, ResetCancellationEvent>::handle(const ResetCancellationEvent&) {
    cancelled = false;
}

#endif
