#ifndef RANDOM_DECISION_GENERATOR_HPP
#define RANDOM_DECISION_GENERATOR_HPP

#include <random>
#include "../interfaces/i_generate_decision.hpp"
#include "../interfaces/i_make_resolution_lineage.hpp"
#include "../interfaces/i_iterate_active_goals.hpp"
#include "../interfaces/i_get_goal_candidate_rule_ids.hpp"

struct random_decision_generator : i_generate_decision {
    random_decision_generator(
        i_make_resolution_lineage& make_resolution_lineage,
        i_iterate_active_goals& iterate_active_goals,
        i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids,
        std::mt19937& rng);
    const resolution_lineage* generate() override;
private:
    const goal_lineage* choose_goal();
    rule_id choose_candidate(const goal_lineage*);

    i_make_resolution_lineage& make_resolution_lineage;
    i_iterate_active_goals& iterate_active_goals;
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids;
    std::mt19937& rng;
};

#endif
