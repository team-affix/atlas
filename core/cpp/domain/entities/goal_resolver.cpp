#include "../../../hpp/domain/entities/goal_resolver.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_resolver::goal_resolver() :
    db(resolver::resolve<i_database>()),
    lp(resolver::resolve<i_lineage_pool>()),
    goal_resolving_producer(resolver::resolve<i_event_producer<goal_resolving_event>>()),
    goal_resolved_producer(resolver::resolve<i_event_producer<goal_resolved_event>>()),
    goal_activating_producer(resolver::resolve<i_event_producer<goal_activating_event>>()),
    goal_activated_producer(resolver::resolve<i_event_producer<goal_activated_event>>()),
    goal_deactivating_producer(resolver::resolve<i_event_producer<goal_deactivating_event>>()),
    goal_deactivated_producer(resolver::resolve<i_event_producer<goal_deactivated_event>>()),
    resolve_yielded_producer(resolver::resolve<i_event_producer<resolve_yielded_event>>()) {}

void goal_resolver::init_resolve(const resolution_lineage* rl) {
    current_rl = rl;
    prev_gl = nullptr;
    body_size = db.at(rl->idx).body.size();
    current_idx = 0;
    deactivating = false;
    goal_resolving_producer.produce({rl});
    resolve_yielded_producer.produce({});
}

void goal_resolver::resume() {
    if (prev_gl) goal_activated_producer.produce({prev_gl});

    if (deactivating) {
        goal_deactivated_producer.produce({current_rl->parent});
        goal_resolved_producer.produce({current_rl});
        // no yield — terminal step
    } else if (current_idx < body_size) {
        prev_gl = lp.goal(current_rl, current_idx);
        goal_activating_producer.produce({prev_gl});
        ++current_idx;
        resolve_yielded_producer.produce({});
    } else {
        goal_deactivating_producer.produce({current_rl->parent});
        deactivating = true;
        resolve_yielded_producer.produce({});
    }
}
