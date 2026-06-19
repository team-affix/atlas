#ifndef CDCL_ELIMINATION_GENERATOR_HPP
#define CDCL_ELIMINATION_GENERATOR_HPP

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include "infrastructure/locator.hpp"
#include "interfaces/i_elimination_generator.hpp"
#include "interfaces/i_learn_avoidance.hpp"
#include "interfaces/i_clean_up_cdcl.hpp"
#include "interfaces/i_try_get_chosen_goal_candidate.hpp"
#include "interfaces/i_set_chosen_goal_candidate.hpp"
#include "infrastructure/coroutine.hpp"
#include "value_objects/avoidance.hpp"

struct cdcl_elimination_generator
    : i_elimination_generator
    , i_learn_avoidance
    , i_clean_up_cdcl {
    cdcl_elimination_generator(locator& loc);
    std::optional<const resolution_lineage*> learn(const lemma&) override;
    coroutine<const resolution_lineage*, void> constrain(const resolution_lineage*) override;
    void cleanup() override;
private:
    using avoidance_id = size_t;

    size_t scan(const avoidance& av) const;
    std::optional<const resolution_lineage*> visit_avoidance(avoidance_id, const resolution_lineage*);

    std::unordered_map<avoidance_id, avoidance> avoidances_;
    size_t next_avoidance_id_ = 0;
    std::unordered_map<const goal_lineage*, std::unordered_set<avoidance_id>> watched_goals_;
    std::unordered_set<avoidance_id> visited_avoidances_;

    i_set_chosen_goal_candidate& set_chosen_goal_candidate_;
    i_try_get_chosen_goal_candidate& try_get_chosen_goal_candidate_;
};

#endif
