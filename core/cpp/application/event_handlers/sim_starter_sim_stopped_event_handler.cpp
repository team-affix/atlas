#include "../../../hpp/application/event_handlers/sim_starter_sim_stopped_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

sim_starter_sim_stopped_event_handler::sim_starter_sim_stopped_event_handler() :
    sim_starter(locator::locate<i_sim_starter>()) {}

void sim_starter_sim_stopped_event_handler::execute(const sim_stopped_event&) {
    sim_starter.start();
}
