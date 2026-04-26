#ifndef AVOIDANCE_HPP
#define AVOIDANCE_HPP

#include <map>
#include "../../value_objects/lineage.hpp"
#include "../../data_structures/lemma.hpp"
#include "../../../infrastructure/event_topic.hpp"
#include "../../events/cdcl_eliminated_candidate_event.hpp"

using avoidance = std::unordered_set<const resolution_lineage*>;

struct cdcl {
    cdcl();
    void learn(const lemma&);
    void constrain(const resolution_lineage*);
    bool refuted() const;
    const std::set<const resolution_lineage*>& get_eliminated_resolutions() const;
#ifndef DEBUG
private:
#endif
    void upsert(size_t, const avoidance&);
    void erase(size_t);

    event_topic<cdcl_eliminated_candidate_event>& cdcl_eliminated_candidate_topic;

    std::map<size_t, avoidance> avoidances;
    std::map<const goal_lineage*, std::set<size_t>> watched_goals;
    bool is_refuted;
    std::set<const resolution_lineage*> eliminated_resolutions;
    size_t next_avoidance_id;
};

#endif
