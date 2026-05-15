#include "../../../hpp/domain/entities/resolver.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

resolver::resolver(size_t initial_goal_count) :
    db(locator::locate<i_database>()),
    lp(locator::locate<i_lineage_pool>()),
    resolving_producer(locator::locate<i_event_producer<resolving_event>>()),
    resolved_producer(locator::locate<i_event_producer<resolved_event>>()),
    goal_activating_producer(locator::locate<i_event_producer<goal_activating_event>>()),
    goal_activated_producer(locator::locate<i_event_producer<goal_activated_event>>()),
    goal_deactivating_producer(locator::locate<i_event_producer<goal_deactivating_event>>()),
    goal_deactivated_producer(locator::locate<i_event_producer<goal_deactivated_event>>()),
    resolve_yielded_producer(locator::locate<i_event_producer<resolve_yielded_event>>()),
    candidate_activating_producer(locator::locate<i_event_producer<candidate_activating_event>>()),
    candidate_activated_producer(locator::locate<i_event_producer<candidate_activated_event>>()),
    candidate_deactivating_producer(locator::locate<i_event_producer<candidate_deactivating_event>>()),
    candidate_deactivated_producer(locator::locate<i_event_producer<candidate_deactivated_event>>()),
    initial_goal_count(initial_goal_count) {}

void resolver::init_resolve(const resolution_lineage* rl) {
    parent_rl = rl;
    body_size = rl ? db.at(rl->idx).body.size() : initial_goal_count;
    
    resolving_producer.produce({rl});
    resolve_yielded_producer.produce({});
}

void resolver::resume() {
    if (finishing)
        finish();
    
    if (activating_candidates)
        resume_activating_candidates();
    else if (activating_subgoals)
        resume_activating_subgoals();
    else if (deactivating_candidates)
        resume_deactivating_candidates();
    else if (deactivating_goal)
        resume_deactivating_goal();

    // always yield unless finishing
    resolve_yielded_producer.produce({});
}

void resolver::resume_activating_subgoals() {
    if (current_gl) goal_activated_producer.produce({current_gl});

    if (subgoal_idx >= body_size && parent_rl) {
        start_deactivating_goal();
        return;
    }

    if (subgoal_idx >= body_size && !parent_rl) {
        start_finishing();
        return;
    }
    
    current_gl = lp.goal(parent_rl, subgoal_idx);
    goal_activating_producer.produce({current_gl});
    ++subgoal_idx;

    start_activating_candidates();
}

void resolver::resume_activating_candidates() {
    if (current_rl) candidate_activated_producer.produce({current_rl});

    if (candidate_idx >= db.size()) {

    }
}

void resolver::resume_deactivating_goal() {
    if (deactivating_goal) {
        goal_deactivated_producer.produce({parent_rl->parent});
        finish();
        return;
    }
    
    deactivating_goal = true;
    goal_deactivating_producer.produce({parent_rl->parent});
    resolve_yielded_producer.produce({});
}

void resolver::finish() {
    resolved_producer.produce({parent_rl});
    // no yield — terminal step
}

void resolver::start_activating_subgoals() {
    activating_subgoals = true;
    current_gl = nullptr;
    subgoal_idx = 0;
}

void resolver::start_activating_candidates() {
    activating_candidates = true;
    current_rl = nullptr;
    candidate_idx = 0;
}

void resolver::start_deactivating_goal() {
    deactivating_goal = true;
}

void resolver::start_deactivating_candidates() {
}

void resolver::start_finishing() {
    finishing = true;
}
