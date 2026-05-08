#ifndef SIM_STOPPER_HPP
#define SIM_STOPPER_HPP

#include <optional>
#include "../interfaces/i_sim_stopper.hpp"
#include "../interfaces/i_cdcl.hpp"
#include "../interfaces/i_decision_store.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/goal_stores_clearing_event.hpp"
#include "../events/sim_stopped_event.hpp"
#include "../value_objects/lemma.hpp"
#include "../../utility/i_trail.hpp"

struct sim_stopper : i_sim_stopper {
    sim_stopper();
    void init_stop() override;
    void finish_stop() override;
private:
    i_trail& trail;
    i_decision_store& decision_store;
    i_cdcl& c;
    i_event_producer<goal_stores_clearing_event>& goal_stores_clearing_producer;
    i_event_producer<sim_stopped_event>& sim_stopped_producer;

    std::optional<lemma> pending_lemma;
};

#endif
