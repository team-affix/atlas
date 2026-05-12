#ifndef ACTIVE_ELIMINATOR_ELIMINATE_CANDIDATE_YIELD_EVENT_HANDLER_HPP
#define ACTIVE_ELIMINATOR_ELIMINATE_CANDIDATE_YIELD_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/eliminate_candidate_yield_event.hpp"
#include "../../domain/interfaces/i_active_eliminator.hpp"

struct active_eliminator_eliminate_candidate_yield_event_handler : event_handler<eliminate_candidate_yield_event> {
    active_eliminator_eliminate_candidate_yield_event_handler();
    void handle(const eliminate_candidate_yield_event&) override;
private:
    i_active_eliminator& active_eliminator;
};

#endif
