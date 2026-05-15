#ifndef MULTIHEAD_UNIFIER_CANDIDATE_DEACTIVATING_EVENT_HANDLER_HPP
#define MULTIHEAD_UNIFIER_CANDIDATE_DEACTIVATING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/candidate_deactivating_event.hpp"
#include "../../domain/interfaces/i_multihead_unifier.hpp"

struct multihead_unifier_candidate_deactivating_event_handler : event_handler<candidate_deactivating_event> {
    virtual ~multihead_unifier_candidate_deactivating_event_handler() = default;
    multihead_unifier_candidate_deactivating_event_handler();
    void handle(const candidate_deactivating_event&) override;
private:
    i_multihead_unifier& multihead_unifier_;
};

#endif
