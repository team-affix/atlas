#ifndef UNIFY_SYNCHRONIZER_RESOLVING_EVENT_HANDLER_HPP
#define UNIFY_SYNCHRONIZER_RESOLVING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/resolving_event.hpp"
#include "../../domain/interfaces/i_unify_synchronizer.hpp"

struct unify_synchronizer_resolving_event_handler : event_handler<resolving_event> {
    unify_synchronizer_resolving_event_handler();
    void handle(const resolving_event&) override;
private:
    i_unify_synchronizer& unify_synchronizer;
};

#endif
