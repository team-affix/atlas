#ifndef LINEAGE_POOL_HPP
#define LINEAGE_POOL_HPP

#include <map>
#include "../domain/interfaces/i_lineage_pool.hpp"

struct lineage_pool : i_lineage_pool {
    const goal_lineage* goal(const resolution_lineage*, size_t) override;
    const resolution_lineage* resolution(const goal_lineage*, size_t) override;
    void pin(const goal_lineage*) override;
    void pin(const resolution_lineage*) override;
    void trim() override;
    const goal_lineage* import(const goal_lineage*) override;
    const resolution_lineage* import(const resolution_lineage*) override;
#ifndef DEBUG
private:
#endif
    const goal_lineage* intern(goal_lineage&&);
    const resolution_lineage* intern(resolution_lineage&&);
    std::map<goal_lineage, bool> goal_lineages;
    std::map<resolution_lineage, bool> resolution_lineages;
};

#endif
