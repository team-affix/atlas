#ifndef AVOIDANCE_HPP
#define AVOIDANCE_HPP

#include <map>
#include "../../value_objects/lineage.hpp"
#include "../../data_structures/lemma.hpp"
#include "../../../infrastructure/event_topic.hpp"
#include "../../events/cdcl_eliminated_candidate_event.hpp"
#include "../../events/conflicted_event.hpp"
#include "../../../infrastructure/tracked_set.hpp"
#include "../../../infrastructure/tracked_map.hpp"
#include "../../../infrastructure/sequencer.hpp"

using avoidance = std::unordered_set<const resolution_lineage*>;

struct cdcl {
    cdcl();
    void learn(const lemma&);
    void constrain(const resolution_lineage*);
    void emit_eliminated_candidates();
    bool check_for_conflict();
#ifndef DEBUG
private:
#endif
    void upsert(size_t, const avoidance&);
    void erase(size_t);

    event_topic<cdcl_eliminated_candidate_event>& cdcl_eliminated_candidate_topic;
    event_topic<conflicted_event>& conflicted_topic;

    tracked_map<std::map<size_t, avoidance>> avoidances;
    tracked_map<std::map<const goal_lineage*, std::set<size_t>>> watched_goals;
    tracked_set<std::set<const resolution_lineage*>> eliminated_resolutions;
    sequencer next_avoidance_id;
};

#endif
