#ifndef GOAL_RESOLVER_HPP
#define GOAL_RESOLVER_HPP

#include "../interfaces/i_goal_resolver.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/goal_resolving_event.hpp"
#include "../events/goal_resolved_event.hpp"
#include "../events/goal_activating_event.hpp"
#include "../events/goal_activated_event.hpp"
#include "../events/goal_deactivating_event.hpp"
#include "../events/goal_deactivated_event.hpp"

struct goal_resolver : i_goal_resolver {
    goal_resolver();
    void resolve(const resolution_lineage*) override;
#ifndef DEBUG
private:
#endif
    i_database& db;
    i_lineage_pool& lp;
    i_event_producer<goal_resolving_event>& goal_resolving_event_producer;
    i_event_producer<goal_resolved_event>& goal_resolved_event_producer;
    i_event_producer<goal_activating_event>& goal_activating_event_producer;
    i_event_producer<goal_activated_event>& goal_activated_event_producer;
    i_event_producer<goal_deactivating_event>& goal_deactivating_event_producer;
    i_event_producer<goal_deactivated_event>& goal_deactivated_event_producer;
};

#endif
