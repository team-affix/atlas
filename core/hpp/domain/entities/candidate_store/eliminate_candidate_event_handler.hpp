#ifndef ELIMINATE_CANDIDATE_EVENT_HANDLER_HPP
#define ELIMINATE_CANDIDATE_EVENT_HANDLER_HPP

#include "../../events/eliminate_candidate_event.hpp"
#include "candidate_store.hpp"

struct eliminate_candidate_event_handler {
    eliminate_candidate_event_handler();
    void operator()(const eliminate_candidate_event& e);
#ifndef DEBUG
private:
#endif
    candidate_store& cs;
};

#endif
