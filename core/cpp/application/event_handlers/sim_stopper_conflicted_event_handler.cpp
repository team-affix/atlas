#include "../../../hpp/application/event_handlers/sim_stopper_conflicted_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

sim_stopper_conflicted_event_handler::sim_stopper_conflicted_event_handler() :
    sim_stopper(locator::resolve<i_sim_stopper>()) {}

void sim_stopper_conflicted_event_handler::handle(const conflicted_event&) {
    sim_stopper.init_stop();
}
