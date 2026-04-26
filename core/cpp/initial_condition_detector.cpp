#include "../hpp/initial_condition_detector.hpp"
#include "../hpp/locator.hpp"

initial_condition_detector::initial_condition_detector() :
    lp(locator::locate<lineage_pool>(locator_keys::inst_lineage_pool)),
    cs(locator::locate<candidate_store>(locator_keys::inst_candidate_store)),
    unit_topic(locator::locate<topic<const resolution_lineage*>>(locator_keys::inst_unit_topic)),
    goal_inserted_subscription(locator::locate<topic<const goal_lineage*>>(locator_keys::inst_goal_inserted_topic)) {
}

bool initial_condition_detector::operator()() {
    while (!goal_inserted_subscription.empty()) {
        const goal_lineage* gl = goal_inserted_subscription.consume();
        const auto& candidates = cs.at(gl);
        if (candidates.empty())
            return true;
        else if (candidates.size() == 1)
            unit_topic.produce(lp.resolution(gl, *candidates.begin()));
    }
    return false;
}
