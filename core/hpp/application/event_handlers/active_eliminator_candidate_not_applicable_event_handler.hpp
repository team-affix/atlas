#ifndef ACTIVE_ELIMINATOR_CANDIDATE_NOT_APPLICABLE_EVENT_HANDLER_HPP
#define ACTIVE_ELIMINATOR_CANDIDATE_NOT_APPLICABLE_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/candidate_not_applicable_event.hpp"
#include "../../domain/interfaces/i_active_eliminator.hpp"

struct active_eliminator_candidate_not_applicable_event_handler : event_handler<candidate_not_applicable_event> {
    active_eliminator_candidate_not_applicable_event_handler();
    void handle(const candidate_not_applicable_event&) override;
private:
    i_active_eliminator& active_eliminator;
};

#endif
