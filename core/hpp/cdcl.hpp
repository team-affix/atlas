#ifndef AVOIDANCE_HPP
#define AVOIDANCE_HPP

#include "lineage.hpp"
#include "defs.hpp"
#include "lemma.hpp"

using avoidance = std::unordered_set<const resolution_lineage*>;

struct cdcl {
    cdcl();
    void learn(const lemma&);
    void constrain(const resolution_lineage*);
    bool refuted() const;
    void set_resolution_eliminated_callback(std::function<void(const resolution_lineage*)>);
    #ifndef DEBUG
    private:
    #endif
    void upsert(size_t, const avoidance&);
    void erase(size_t);

    std::map<size_t, avoidance> avoidances;
    std::map<const goal_lineage*, std::set<size_t>> watched_goals;
    bool is_refuted;
    std::set<const resolution_lineage*> eliminated_resolutions;
    size_t next_avoidance_id;
    std::function<void(const resolution_lineage*)> resolution_eliminated_callback;
};

#endif
