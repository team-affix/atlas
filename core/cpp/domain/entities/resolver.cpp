#include "../../../hpp/domain/entities/resolver.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

resolver::resolver(size_t initial_goal_count) :
    db(locator::resolve<i_database>()),
    lp(locator::resolve<i_lineage_pool>()),
    resolving_producer(locator::resolve<i_event_producer<resolving_event>>()),
    resolved_producer(locator::resolve<i_event_producer<resolved_event>>()),
    goal_activating_producer(locator::resolve<i_event_producer<goal_activating_event>>()),
    goal_activated_producer(locator::resolve<i_event_producer<goal_activated_event>>()),
    goal_deactivating_producer(locator::resolve<i_event_producer<goal_deactivating_event>>()),
    goal_deactivated_producer(locator::resolve<i_event_producer<goal_deactivated_event>>()),
    resolve_yielded_producer(locator::resolve<i_event_producer<resolve_yielded_event>>()),
    initial_goal_count(initial_goal_count) {}

void resolver::init_resolve(const resolution_lineage* rl) {
    current_rl = rl;
    prev_gl = nullptr;
    body_size = rl ? db.at(rl->idx).body.size() : initial_goal_count;
    current_idx = 0;
    deactivating = false;
    resolving_producer.produce({rl});
    resolve_yielded_producer.produce({});
}

void resolver::resume() {
    if (prev_gl) goal_activated_producer.produce({prev_gl});

    if (deactivating) {
        goal_deactivated_producer.produce({current_rl->parent});
        resolved_producer.produce({current_rl});
        // no yield — terminal step
    } else if (current_idx < body_size) {
        prev_gl = lp.goal(current_rl, current_idx);
        goal_activating_producer.produce({prev_gl});
        ++current_idx;
        resolve_yielded_producer.produce({});
    } else {
        if (!current_rl) { resolved_producer.produce({nullptr}); return; }
        goal_deactivating_producer.produce({current_rl->parent});
        deactivating = true;
        resolve_yielded_producer.produce({});
    }
}
