#include "../../../../hpp/domain/entities/initial_condition_detector/icd_goal_inserted_event_handler.hpp"
#include "../../../../hpp/infrastructure/locator.hpp"

icd_goal_inserted_event_handler::icd_goal_inserted_event_handler() :
    icd(locator::locate<initial_condition_checker>()) {
}

void icd_goal_inserted_event_handler::operator()(const goal_inserted_event& e) {
    icd(e.gl);
}
