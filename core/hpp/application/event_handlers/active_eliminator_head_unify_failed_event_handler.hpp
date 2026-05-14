#ifndef ACTIVE_ELIMINATOR_HEAD_UNIFY_FAILED_EVENT_HANDLER_HPP
#define ACTIVE_ELIMINATOR_HEAD_UNIFY_FAILED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/head_unify_failed_event.hpp"
#include "../../domain/interfaces/i_active_eliminator.hpp"

struct active_eliminator_head_unify_failed_event_handler : event_handler<head_unify_failed_event> {
    active_eliminator_head_unify_failed_event_handler();
    void handle(const head_unify_failed_event&) override;
private:
    i_active_eliminator& active_eliminator;
};

#endif
