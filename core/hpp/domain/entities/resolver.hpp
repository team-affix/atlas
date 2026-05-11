#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include "../interfaces/i_resolver.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/resolving_event.hpp"
#include "../events/resolved_event.hpp"
#include "../events/goal_activating_event.hpp"
#include "../events/goal_activated_event.hpp"
#include "../events/goal_deactivating_event.hpp"
#include "../events/goal_deactivated_event.hpp"
#include "../events/resolve_yielded_event.hpp"

struct resolver : i_resolver {
    explicit resolver(size_t initial_goal_count);
    void init_resolve(const resolution_lineage*) override;
    void resume() override;
private:
    i_database& db;
    i_lineage_pool& lp;
    i_event_producer<resolving_event>& resolving_producer;
    i_event_producer<resolved_event>& resolved_producer;
    i_event_producer<goal_activating_event>& goal_activating_producer;
    i_event_producer<goal_activated_event>& goal_activated_producer;
    i_event_producer<goal_deactivating_event>& goal_deactivating_producer;
    i_event_producer<goal_deactivated_event>& goal_deactivated_producer;
    i_event_producer<resolve_yielded_event>& resolve_yielded_producer;

    size_t initial_goal_count;
    const resolution_lineage* current_rl = nullptr;
    const goal_lineage* prev_gl = nullptr;
    size_t current_idx = 0;
    size_t body_size = 0;
    bool deactivating = false;
};

#endif
