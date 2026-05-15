#ifndef MULTIHEAD_UNIFIER_RESOLVING_EVENT_HANDLER_HPP
#define MULTIHEAD_UNIFIER_RESOLVING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/resolving_event.hpp"
#include "../../domain/interfaces/i_multihead_unifier.hpp"

struct multihead_unifier_resolving_event_handler : event_handler<resolving_event> {
    virtual ~multihead_unifier_resolving_event_handler() = default;
    multihead_unifier_resolving_event_handler();
    void handle(const resolving_event&) override;
private:
    i_multihead_unifier& multihead_unifier_;
};

#endif
