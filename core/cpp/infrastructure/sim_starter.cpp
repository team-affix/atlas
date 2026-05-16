#include "../../../hpp/domain/entities/sim_starter.hpp"
#include "../../../hpp/domain/events/fixpoint_reached_event.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

sim_starter::sim_starter() :
    trail(locator::locate<i_trail>()),
    res(locator::locate<i_resolver>()),
    sim_started_producer(locator::locate<i_event_producer<sim_started_event>>()),
    fixpoint_reached_producer(locator::locate<i_event_producer<fixpoint_reached_event>>()) {}

void sim_starter::start() {
    trail.push();
    res.init_resolve(nullptr);
    sim_started_producer.produce({});
    fixpoint_reached_producer.produce({});
}
