#ifndef CDCL_ELIMINATION_GENERATOR_HPP
#define CDCL_ELIMINATION_GENERATOR_HPP

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include "infrastructure/locator.hpp"
#include "interfaces/i_elimination_generator.hpp"
#include "interfaces/i_learn_avoidance.hpp"
#include "interfaces/i_cdcl_sequencer.hpp"
#include "infrastructure/coroutine.hpp"
#include "infrastructure/tracked.hpp"

struct cdcl_elimination_generator
    : i_elimination_generator
    , i_learn_avoidance {
    cdcl_elimination_generator(locator& loc);
    std::optional<const resolution_lineage*> learn(const lemma&) override;
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*) override;
private:
    using avoidance_id = size_t;
    using avoidance_type = std::unordered_set<const resolution_lineage*>;
    using avoidances_type = std::unordered_map<avoidance_id, avoidance_type>;
    using watched_goals_type = std::unordered_map<const goal_lineage*, std::unordered_set<avoidance_id>>;

    std::optional<const resolution_lineage*> insert(avoidance_type av);
    void link(const goal_lineage*, avoidance_id);
    void erase(avoidance_id);

    tracked<avoidances_type> avoidances;
    tracked<watched_goals_type> watched_goals;
    i_cdcl_sequencer& cdcl_sequencer;
};

#endif
