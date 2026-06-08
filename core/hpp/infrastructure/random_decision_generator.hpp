#ifndef RANDOM_DECISION_GENERATOR_HPP
#define RANDOM_DECISION_GENERATOR_HPP

#include <random>
#include "infrastructure/locator.hpp"
#include "interfaces/i_generate_decision.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_random_access.hpp"
#include "interfaces/i_active_goals_size.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_ra_rule_id_set.hpp"

struct random_decision_generator : i_generate_decision {
    random_decision_generator(locator& loc, std::mt19937& rng);
    const resolution_lineage* generate() override;
private:
    const goal_lineage* choose_goal();
    rule_id choose_candidate(const goal_lineage*);

    i_make_resolution_lineage& make_resolution_lineage;
    i_random_access<const goal_lineage*>& goal_random_access;
    i_active_goals_size& active_goals_size;
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids;
    std::mt19937& rng;
};

#endif
