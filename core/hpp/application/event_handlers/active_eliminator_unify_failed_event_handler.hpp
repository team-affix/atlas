#ifndef ACTIVE_ELIMINATOR_UNIFY_FAILED_EVENT_HANDLER_HPP
#define ACTIVE_ELIMINATOR_UNIFY_FAILED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/unify_failed_event.hpp"
#include "../../domain/interfaces/i_active_eliminator.hpp"

struct active_eliminator_unify_failed_event_handler : event_handler<unify_failed_event> {
    active_eliminator_unify_failed_event_handler();
    void handle(const unify_failed_event&) override;
private:
    i_active_eliminator& active_eliminator;
};

#endif
