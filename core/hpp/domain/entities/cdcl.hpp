#ifndef CDCL_HPP
#define CDCL_HPP

#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_cdcl.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../interfaces/i_cdcl_sequencer.hpp"
#include "../events/avoidance_unit_event.hpp"
#include "../events/avoidance_empty_event.hpp"
#include "../../utility/tracked.hpp"

struct cdcl : i_cdcl {
    cdcl();
    void learn(const lemma&) override;
    void constrain(const resolution_lineage*) override;
    const avoidance& get_avoidance(size_t) override;
private:
    void updated(size_t);
    void link(const goal_lineage*, size_t);
    void erase(size_t);

    i_event_producer<avoidance_unit_event>& avoidance_unit_producer;
    i_event_producer<avoidance_empty_event>& avoidance_empty_producer;
    i_cdcl_sequencer& next_avoidance_id;

    using avoidances_type = std::unordered_map<size_t, avoidance>;
    using watched_goals_type = std::unordered_map<const goal_lineage*, std::unordered_set<size_t>>;
    
    tracked<avoidances_type> avoidances;
    tracked<watched_goals_type> watched_goals;
};

#endif
