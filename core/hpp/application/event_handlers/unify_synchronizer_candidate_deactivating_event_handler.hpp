#ifndef UNIFY_SYNCHRONIZER_CANDIDATE_DEACTIVATING_EVENT_HANDLER_HPP
#define UNIFY_SYNCHRONIZER_CANDIDATE_DEACTIVATING_EVENT_HANDLER_HPP

#include "../../infrastructure/event_handler.hpp"
#include "../../domain/events/candidate_deactivating_event.hpp"
#include "../../domain/interfaces/i_unify_synchronizer.hpp"

struct unify_synchronizer_candidate_deactivating_event_handler : event_handler<candidate_deactivating_event> {
    unify_synchronizer_candidate_deactivating_event_handler();
    void handle(const candidate_deactivating_event&) override;
private:
    i_unify_synchronizer& unify_synchronizer;
};

#endif
