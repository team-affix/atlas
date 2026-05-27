#ifndef RANDOM_DECISION_GENERATOR_HPP
#define RANDOM_DECISION_GENERATOR_HPP

#include <random>
#include "../interfaces/i_decision_generator.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_iterate_active_goals.hpp"
#include "../interfaces/i_get_goal_candidate_rules.hpp"

struct random_decision_generator : i_decision_generator {
    random_decision_generator(
        i_lineage_pool& lp,
        i_iterate_active_goals& iterate_active_goals,
        i_get_goal_candidate_rules& ggcr,
        std::mt19937& rng);
    const resolution_lineage* generate() override;
private:
    const goal_lineage* choose_goal();
    const rule* choose_candidate(const goal_lineage*);

    i_lineage_pool& lp;
    i_iterate_active_goals& iterate_active_goals;
    i_get_goal_candidate_rules& ggcr;
    std::mt19937& rng;
};

#endif
