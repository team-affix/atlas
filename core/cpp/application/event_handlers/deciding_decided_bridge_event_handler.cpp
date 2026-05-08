#include "../../../hpp/application/event_handlers/deciding_decided_bridge_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

deciding_decided_bridge_event_handler::deciding_decided_bridge_event_handler() :
    decided_producer(resolver::resolve<i_event_producer<decided_event>>()) {}

void deciding_decided_bridge_event_handler::handle(const deciding_event& e) {
    decided_producer.produce(decided_event{e.rl});
}
