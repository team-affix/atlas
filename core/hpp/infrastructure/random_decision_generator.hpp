#ifndef RANDOM_DECISION_GENERATOR_HPP
#define RANDOM_DECISION_GENERATOR_HPP

#include <random>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IMakeResolutionLineage, typename ISelectActiveGoal, typename IGetGoalCandidateRuleIds>
struct random_decision_generator {
    random_decision_generator(IMakeResolutionLineage&, ISelectActiveGoal&, IGetGoalCandidateRuleIds&,
                              std::mt19937& rng);
    const resolution_lineage* generate();
private:
    const goal_lineage* choose_goal();
    rule_id choose_candidate(const goal_lineage*);
    IMakeResolutionLineage& make_resolution_lineage_;
    ISelectActiveGoal& goal_random_access_;
    IGetGoalCandidateRuleIds& get_goal_candidate_rule_ids_;
    std::mt19937& rng_;
};

template<typename IMRL, typename ISAG, typename IGCRI>
random_decision_generator<IMRL, ISAG, IGCRI>::random_decision_generator(
    IMRL& lp, ISAG& ag, IGCRI& gcr, std::mt19937& rng)
    : make_resolution_lineage_(lp), goal_random_access_(ag), get_goal_candidate_rule_ids_(gcr), rng_(rng) {}

template<typename IMRL, typename ISAG, typename IGCRI>
const resolution_lineage* random_decision_generator<IMRL, ISAG, IGCRI>::generate() {
    const goal_lineage* gl = choose_goal();
    rule_id r = choose_candidate(gl);
    return make_resolution_lineage_.make_resolution_lineage(gl, r);
}

template<typename IMRL, typename ISAG, typename IGCRI>
const goal_lineage* random_decision_generator<IMRL, ISAG, IGCRI>::choose_goal() {
    size_t n = goal_random_access_.active_goals_size();
    std::uniform_int_distribution<size_t> dist(0, n - 1);
    return goal_random_access_.select(dist(rng_));
}

template<typename IMRL, typename ISAG, typename IGCRI>
rule_id random_decision_generator<IMRL, ISAG, IGCRI>::choose_candidate(
    const goal_lineage* gl) {
    auto& rules = get_goal_candidate_rule_ids_.get(gl);
    std::uniform_int_distribution<size_t> dist(0, rules.size() - 1);
    return rules.select(dist(rng_));
}

#endif
