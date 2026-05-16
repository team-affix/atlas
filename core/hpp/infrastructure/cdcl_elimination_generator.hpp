#ifndef CDCL_ELIMINATION_GENERATOR_HPP
#define CDCL_ELIMINATION_GENERATOR_HPP

#include <set>
#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_cdcl_elimination_generator.hpp"
#include "../utility/state_machine.hpp"
#include "../utility/tracked.hpp"

struct cdcl_elimination_generator : i_cdcl_elimination_generator {
    cdcl_elimination_generator(
        i_trail&
    );
    const resolution_lineage* learn(const lemma&) override;
    state_machine<const resolution_lineage*> constrain(const resolution_lineage*) override;
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
