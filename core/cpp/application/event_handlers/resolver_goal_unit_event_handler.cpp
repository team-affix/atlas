#include "../../../hpp/application/event_handlers/resolver_goal_unit_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

resolver_goal_unit_event_handler::resolver_goal_unit_event_handler() :
    res(locator::locate<i_resolver>()),
    frontier(locator::locate<i_frontier>()),
    lp(locator::locate<i_lineage_pool>()) {
}

void resolver_goal_unit_event_handler::execute(const goal_unit_event& e) {
    const auto& candidates = frontier.at(e.gl)->candidates;
    const auto& candidate = candidates.begin();
    res.init_resolve(lp.resolution(e.gl, candidate->first));
}
