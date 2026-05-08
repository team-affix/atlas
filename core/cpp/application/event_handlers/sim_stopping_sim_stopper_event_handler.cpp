#include "../../../hpp/application/event_handlers/sim_stopping_sim_stopper_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

sim_stopping_sim_stopper_event_handler::sim_stopping_sim_stopper_event_handler() :
    sim_stopper(resolver::resolve<i_sim_stopper>()) {}

void sim_stopping_sim_stopper_event_handler::handle(const sim_stopping_event&) {
    sim_stopper.init_stop();
}
