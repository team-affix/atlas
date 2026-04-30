#ifndef AVOIDANCE_HPP
#define AVOIDANCE_HPP

#include <map>
#include <memory>
#include "../interfaces/i_cdcl.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../interfaces/i_tracked.hpp"
#include "../events/avoidance_is_unit_event.hpp"
#include "../events/avoidance_is_empty_event.hpp"

struct cdcl : i_cdcl {
    cdcl();
    void learn(const lemma&) override;
    void constrain(const resolution_lineage*) override;
    avoidance get_avoidance(size_t) override;
    void produce_events() override;
#ifndef DEBUG
private:
#endif
    void upsert(size_t, const avoidance&);
    void erase(size_t);

    i_event_producer<avoidance_is_unit_event>& avoidance_is_unit_producer;
    i_event_producer<avoidance_is_empty_event>& avoidance_is_empty_producer;

    tracked_map<std::map<size_t, avoidance>> avoidances;
    std::unique_ptr<i_tracked<std::map<const goal_lineage*, std::set<size_t>>>> watched_goals;
    tracked_set<std::set<const resolution_lineage*>> eliminated_resolutions;
    sequencer next_avoidance_id;
};

#endif
