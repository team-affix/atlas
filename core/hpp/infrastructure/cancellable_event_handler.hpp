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
    void operator()(const Event&) final override;
    void operator()(const CancellationEvent&) final override;
    void operator()(const ResetCancellationEvent&) final override;
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
void cancellable_event_handler<Event, CancellationEvent, ResetCancellationEvent>::operator()(const Event& event) {
    if (cancelled)
        return;
    execute(event);
}

template<typename Event, typename CancellationEvent, typename ResetCancellationEvent>
void cancellable_event_handler<Event, CancellationEvent, ResetCancellationEvent>::operator()(const CancellationEvent& event) {
    cancelled = true;
}

template<typename Event, typename CancellationEvent, typename ResetCancellationEvent>
void cancellable_event_handler<Event, CancellationEvent, ResetCancellationEvent>::operator()(const ResetCancellationEvent& event) {
    cancelled = false;
}

#endif
