#ifndef RANDOM_DECISION_GENERATOR_HPP
#define RANDOM_DECISION_GENERATOR_HPP

#include <random>
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename ILineagePool, typename IActiveGoals, typename IGoalCandidateRules>
struct random_decision_generator {
    random_decision_generator(ILineagePool&, IActiveGoals&, IGoalCandidateRules&,
                              std::mt19937& rng);
    const resolution_lineage* generate();
private:
    const goal_lineage* choose_goal();
    rule_id choose_candidate(const goal_lineage*);
    ILineagePool& make_resolution_lineage;
    IActiveGoals& goal_random_access;
    IGoalCandidateRules& get_goal_candidate_rule_ids;
    std::mt19937& rng;
};

template<typename ILP, typename IAG, typename IGCR>
random_decision_generator<ILP, IAG, IGCR>::random_decision_generator(
    ILP& lp, IAG& ag, IGCR& gcr, std::mt19937& rng)
    : make_resolution_lineage(lp), goal_random_access(ag),
      get_goal_candidate_rule_ids(gcr), rng(rng) {}

template<typename ILP, typename IAG, typename IGCR>
const resolution_lineage* random_decision_generator<ILP, IAG, IGCR>::generate() {
    const goal_lineage* gl = choose_goal();
    rule_id r = choose_candidate(gl);
    return make_resolution_lineage.make_resolution_lineage(gl, r);
}

template<typename ILP, typename IAG, typename IGCR>
const goal_lineage* random_decision_generator<ILP, IAG, IGCR>::choose_goal() {
    size_t n = goal_random_access.active_goals_size();
    std::uniform_int_distribution<size_t> dist(0, n - 1);
    return goal_random_access.select(dist(rng));
}

template<typename ILP, typename IAG, typename IGCR>
rule_id random_decision_generator<ILP, IAG, IGCR>::choose_candidate(
    const goal_lineage* gl) {
    auto& rules = get_goal_candidate_rule_ids.get(gl);
    std::uniform_int_distribution<size_t> dist(0, rules.size() - 1);
    return rules.select(dist(rng));
}

#endif
