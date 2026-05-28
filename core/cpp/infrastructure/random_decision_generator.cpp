#include "../../hpp/infrastructure/random_decision_generator.hpp"

random_decision_generator::random_decision_generator(
    i_make_resolution_lineage& make_resolution_lineage,
    i_iterate_active_goals& iterate_active_goals,
    i_get_goal_candidate_rules& ggcr,
    std::mt19937& rng)
    :
    make_resolution_lineage(make_resolution_lineage),
    iterate_active_goals(iterate_active_goals),
    ggcr(ggcr),
    rng(rng) {}

const resolution_lineage* random_decision_generator::generate() {
    const goal_lineage* gl = choose_goal();
    rule_id r = choose_candidate(gl);
    return make_resolution_lineage.make(gl, r);
}

const goal_lineage* random_decision_generator::choose_goal() {
    std::vector<const goal_lineage*> goals;
    auto it = iterate_active_goals.iterate_active_goals();
    while (!it.done()) {
        auto gl = it.resume();
        if (!gl.has_value())
            continue;
        goals.push_back(gl.value());
    }

    std::uniform_int_distribution<size_t> dist(0, goals.size() - 1);
    return goals[dist(rng)];
}

rule_id random_decision_generator::choose_candidate(const goal_lineage* gl) {
    std::vector<rule_id> candidates;
    auto& rules = ggcr.get(gl);
    auto it = rules.iterate();
    while (!it.done()) {
        auto r = it.resume();
        if (!r.has_value())
            continue;
        candidates.push_back(r.value());
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
}
