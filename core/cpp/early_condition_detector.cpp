#include "../hpp/early_condition_detector.hpp"

early_condition_detector::early_condition_detector(
    candidate_store& cs,
    lineage_pool& lp,
    topic<const goal_lineage*>& goal_inserted_topic,
    topic<const resolution_lineage*>& unit_topic
) :
    lp(lp),
    cs(cs),
    unit_topic(unit_topic),
    goal_inserted_subscription(goal_inserted_topic) {

}

bool early_condition_detector::operator()() {
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
