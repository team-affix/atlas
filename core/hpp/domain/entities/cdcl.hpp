#ifndef CDCL_HPP
#define CDCL_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_cdcl.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/cdcl_constrain_yielded_event.hpp"
#include "../../utility/state_machine.hpp"
#include "../../utility/tracked.hpp"

struct cdcl : i_cdcl {
    cdcl();
    void learn(const lemma&) override;
    void init_constrain(const resolution_lineage*) override;
    void resume_constrain() override;
    bool contains(const avoidance_type&) override;
private:
    void link(const goal_lineage*, size_t);
    void erase(size_t);
    state_machine<void> constrain(const resolution_lineage*);

    i_event_producer<cdcl_constrain_yielded_event>& cdcl_constrain_yielded_producer;

    using avoidances_type = std::set<avoidance_type>;
    using contraction_map_type = std::unordered_map<const avoidance_type*, std::unordered_set<const goal_lineage*>>;
    using watched_goals_type = std::unordered_map<const goal_lineage*, std::unordered_set<const avoidances_type*>>;
    
    std::optional<state_machine<void>> constrain_state_machine;
    
    tracked<avoidances_type> avoidances;
    tracked<contraction_map_type> contraction_map;
    tracked<watched_goals_type> watched_goals;
};

#endif
