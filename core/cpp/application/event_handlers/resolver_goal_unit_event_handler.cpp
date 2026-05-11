#include "../../../hpp/application/event_handlers/resolver_goal_unit_event_handler.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

resolver_goal_unit_event_handler::resolver_goal_unit_event_handler() :
    res(locator::locate<i_resolver>()),
    candidates_frontier(locator::locate<i_candidates_frontier>()),
    lp(locator::locate<i_lineage_pool>()) {
}

void resolver_goal_unit_event_handler::execute(const goal_unit_event& e) {
    const auto& candidates = candidates_frontier.at(e.gl);
    const auto& candidate = *candidates.candidates.begin();
    res.init_resolve(lp.resolution(e.gl, candidate));
}
