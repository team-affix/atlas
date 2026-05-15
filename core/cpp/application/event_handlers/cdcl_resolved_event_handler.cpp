#include "../../../hpp/application/event_handlers/cdcl_resolved_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

cdcl_resolved_event_handler::cdcl_resolved_event_handler() :
    cdcl(locator::locate<i_cdcl>()) {}

void cdcl_resolved_event_handler::handle(const resolved_event& e) {
    if (!e.rl) { return; }
    cdcl.constrain(e.rl);
}
