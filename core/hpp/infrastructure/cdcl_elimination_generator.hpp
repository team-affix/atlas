#ifndef CDCL_ELIMINATION_GENERATOR_HPP
#include "infrastructure/locator.hpp"

#define CDCL_ELIMINATION_GENERATOR_HPP

#include <set>
#include <unordered_map>
#include <unordered_set>
#include "interfaces/i_elimination_generator.hpp"
#include "interfaces/i_learn_avoidance.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/tracked.hpp"

struct cdcl_elimination_generator
    : i_elimination_generator
    , i_learn_avoidance {
    cdcl_elimination_generator(locator& loc);
    std::optional<const resolution_lineage*> learn(const lemma&) override;
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*) override;
private:
    using avoidance_type = std::set<const resolution_lineage*>;
    const resolution_lineage* insert(const avoidance_type&);
    void link(const goal_lineage*, const avoidance_type*);
    void erase(const avoidance_type*);
    
    using watched_goals_type = std::unordered_map<const goal_lineage*, std::unordered_set<const avoidance_type*>>;
        
    tracked<std::set<avoidance_type>> avoidances;
    tracked<watched_goals_type> watched_goals;
};

#endif
