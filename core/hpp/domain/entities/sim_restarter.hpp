#ifndef SIM_RESTARTER_HPP
#define SIM_RESTARTER_HPP

#include <optional>
#include "../interfaces/i_sim_restarter.hpp"
#include "../interfaces/i_cdcl.hpp"
#include "../interfaces/i_decision_store.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../interfaces/i_initial_goal_activator.hpp"
#include "../events/goal_stores_clearing_event.hpp"
#include "../value_objects/lemma.hpp"
#include "../../utility/i_trail.hpp"

struct sim_restarter : i_sim_restarter {
    sim_restarter();
    void begin_restart() override;
    void complete_restart() override;
private:
    i_trail& trail;
    i_decision_store& decision_store;
    i_cdcl& c;
    i_initial_goal_activator& initial_goal_activator;
    i_event_producer<goal_stores_clearing_event>& goal_stores_clearing_producer;

    std::optional<lemma> pending_lemma;
};

#endif
