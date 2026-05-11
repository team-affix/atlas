#ifndef CANDIDATE_APPLICANT_ACTIVATOR_CANDIDATE_ACTIVATED_EVENT_HANDLER_HPP
#define CANDIDATE_APPLICANT_ACTIVATOR_CANDIDATE_ACTIVATED_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/candidate_activated_event.hpp"
#include "../../domain/interfaces/i_candidate_applicant_activator.hpp"

struct candidate_applicant_activator_candidate_activated_event_handler : event_handler<candidate_activated_event> {
    candidate_applicant_activator_candidate_activated_event_handler();
    void handle(const candidate_activated_event&) override;
private:
    i_candidate_applicant_activator& candidate_applicant_activator;
};

#endif
