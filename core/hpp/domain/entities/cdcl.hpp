#ifndef CDCL_HPP
#define CDCL_HPP

#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_cdcl.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/avoidance_is_unit_event.hpp"
#include "../events/avoidance_is_empty_event.hpp"
#include "../../utility/tracked.hpp"

struct cdcl : i_cdcl {
    cdcl();
    void learn(const lemma&) override;
    void constrain(const resolution_lineage*) override;
    void produce_events() override;
    const avoidance& get_avoidance(size_t) override;
#ifndef DEBUG
private:
#endif
    void upsert(size_t, const avoidance&);
    void erase(size_t);

    i_event_producer<avoidance_is_unit_event>& avoidance_is_unit_producer;
    i_event_producer<avoidance_is_empty_event>& avoidance_is_empty_producer;

    tracked<std::unordered_map<size_t, avoidance>> avoidances;
    tracked<std::unordered_map<const goal_lineage*, std::unordered_set<size_t>>> watched_goals;
    tracked<std::unordered_set<size_t>> unit_avoidances;
    tracked<std::unordered_set<size_t>> empty_avoidances;
    sequencer next_avoidance_id;
};

#endif
