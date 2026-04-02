#ifndef AVOIDANCE_HPP
#define AVOIDANCE_HPP

#include "lineage.hpp"
#include "defs.hpp"

using avoidance = std::unordered_set<const resolution_lineage*>;

struct cdcl {
    cdcl();
    void learn(const decisions&);
    void constrain(const resolution_lineage*);
    bool refuted() const;
    bool eliminated(const resolution_lineage*) const;
    #ifndef DEBUG
    private:
    #endif
    void upsert(size_t, const avoidance&);
    void erase(size_t);
    static avoidance reduce(const decisions&);
    static void remove_ancestors(const resolution_lineage*, avoidance&, std::set<const resolution_lineage*>&);

    std::map<size_t, avoidance> avoidances;
    std::map<const goal_lineage*, std::set<size_t>> watched_goals;
    bool is_refuted;
    std::set<const resolution_lineage*> eliminated_resolutions;
    size_t next_avoidance_id;
};

#endif
