#ifndef AVOIDANCE_HPP
#define AVOIDANCE_HPP

#include "lineage.hpp"
#include "lemma.hpp"
#include "topic.hpp"

using avoidance = std::unordered_set<const resolution_lineage*>;

struct cdcl {
    cdcl(topic<const resolution_lineage*>&);
    void learn(const lemma&);
    void constrain(const resolution_lineage*);
    bool refuted() const;
    const std::set<const resolution_lineage*>& get_eliminated_resolutions() const;
    #ifndef DEBUG
    private:
    #endif
    void upsert(size_t, const avoidance&);
    void erase(size_t);

    topic<const resolution_lineage*>& new_eliminated_resolution_topic;

    std::map<size_t, avoidance> avoidances;
    std::map<const goal_lineage*, std::set<size_t>> watched_goals;
    bool is_refuted;
    std::set<const resolution_lineage*> eliminated_resolutions;
    size_t next_avoidance_id;
};

#endif
