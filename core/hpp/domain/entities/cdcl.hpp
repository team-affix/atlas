#ifndef CDCL_HPP
#define CDCL_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_cdcl.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../interfaces/i_cdcl_sequencer.hpp"
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
    void updated(size_t);
    void link(const goal_lineage*, size_t);
    void erase(size_t);
    state_machine constrain(const resolution_lineage*);

    i_event_producer<cdcl_constrain_yielded_event>& cdcl_constrain_yielded_producer;
    i_cdcl_sequencer& next_avoidance_id;

    using avoidances_type = std::unordered_map<size_t, avoidance_type>;
    
    using watched_goals_type = std::unordered_map<const goal_lineage*, std::unordered_set<size_t>>;
    
    std::optional<state_machine> constrain_state_machine;
    
    tracked<avoidances_type> avoidances;
    tracked<watched_goals_type> watched_goals;
};

#endif
