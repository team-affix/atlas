#include "../../../hpp/application/event_handlers/goal_resolver_goal_unit_event_handler.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_resolver_goal_unit_event_handler::goal_resolver_goal_unit_event_handler() :
    goal_resolver(resolver::resolve<i_goal_resolver>()),
    goal_candidates_store(resolver::resolve<i_goal_candidates_store>()),
    lp(resolver::resolve<i_lineage_pool>()) {
}

void goal_resolver_goal_unit_event_handler::execute(const goal_unit_event& e) {
    // get candidates
    const auto& candidates = goal_candidates_store.at(e.gl);

    // get only remaining candidate
    const auto& candidate = *candidates.candidates.begin();

    // resolve goal
    goal_resolver.resolve(lp.resolution(e.gl, candidate));
}
