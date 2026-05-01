#ifndef LINEAGE_POOL_HPP
#define LINEAGE_POOL_HPP

#include <map>
#include "../value_objects/lineage.hpp"

struct lineage_pool {
    const goal_lineage* goal(const resolution_lineage*, size_t);
    const resolution_lineage* resolution(const goal_lineage*, size_t);
    void pin(const goal_lineage*);
    void pin(const resolution_lineage*);
    void trim();
    const goal_lineage* import(const goal_lineage*);
    const resolution_lineage* import(const resolution_lineage*);
#ifndef DEBUG
private:
#endif
    const goal_lineage* intern(goal_lineage&&);
    const resolution_lineage* intern(resolution_lineage&&);
    std::map<goal_lineage, bool> goal_lineages;
    std::map<resolution_lineage, bool> resolution_lineages;
};

#endif
