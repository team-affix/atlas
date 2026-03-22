#ifndef AVOIDANCE_HPP
#define AVOIDANCE_HPP

#include <set>
#include "lineage.hpp"
#include "defs.hpp"

using avoidance = std::set<const resolution_lineage*>;

struct cdcl {
    cdcl();
    size_t learn(const decision_store&);
    void constrain(const resolution_lineage*);
    bool refuted() const;
    bool eliminated(const resolution_lineage*) const;
    #ifndef DEBUG
    private:
    #endif
    void upsert(size_t, const avoidance&);
    void erase(size_t);
    static avoidance reduce(const decision_store&);
    static void remove_ancestors(const resolution_lineage*, avoidance&, std::set<const resolution_lineage*>&);

    std::map<size_t, avoidance> avoidances;
    std::map<const goal_lineage*, std::set<size_t>> watched_goals;
    bool is_refuted;
    std::set<const resolution_lineage*> eliminated_resolutions;
};

#endif
