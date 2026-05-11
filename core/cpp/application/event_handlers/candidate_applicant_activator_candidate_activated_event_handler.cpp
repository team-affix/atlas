#include "../../../hpp/application/event_handlers/candidate_applicant_activator_candidate_activated_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

candidate_applicant_activator_candidate_activated_event_handler::candidate_applicant_activator_candidate_activated_event_handler() :
    candidate_applicant_activator(locator::locate<i_candidate_applicant_activator>()) {}

void candidate_applicant_activator_candidate_activated_event_handler::handle(const candidate_activated_event& e) {
    candidate_applicant_activator.activate(e.rl);
}
