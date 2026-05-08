#include "../../../hpp/application/event_handlers/avoidance_empty_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

avoidance_empty_event_handler::avoidance_empty_event_handler() :
    get_sim_active(resolver::resolve<i_get_sim_active>()),
    conflicted_producer(resolver::resolve<i_event_producer<conflicted_event>>()),
    refuted_producer(resolver::resolve<i_event_producer<refuted_event>>()) {
}

void avoidance_empty_event_handler::execute(const avoidance_empty_event& event) {
    if (get_sim_active.get_sim_active())
        conflicted_producer.produce(conflicted_event{});
    else
        refuted_producer.produce(refuted_event{});
}
