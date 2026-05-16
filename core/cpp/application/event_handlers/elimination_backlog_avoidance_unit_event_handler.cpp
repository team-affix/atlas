#include "../../../hpp/application/event_handlers/elimination_backlog_avoidance_unit_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

elimination_backlog_avoidance_unit_event_handler::elimination_backlog_avoidance_unit_event_handler() :
    c(locator::locate<i_cdcl>()),
    elimination_backlog(locator::locate<i_elimination_backlog>()) {
}

void elimination_backlog_avoidance_unit_event_handler::execute(const avoidance_unit_event& e) {
    // get the avoidance
    const i_cdcl::avoidance_type& av = c.get_avoidance(e.avoidance_id);
    
    // get the first resolution
    const resolution_lineage* rl = *av.begin();
    
    // get the parent goal
    const goal_lineage* gl = rl->parent;

    // insert the resolution into the elimination backlog
    elimination_backlog.insert(rl);
}
