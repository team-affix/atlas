#include "../hpp/a01_decider.hpp"

a01_decider::a01_decider(
    const a01_goal_store& gs,
    const a01_candidate_store& cs,
    monte_carlo::simulation<choice, std::mt19937>& sim
)
    : gs(gs), cs(cs), sim(sim)
{}

std::pair<const goal_lineage*, size_t> a01_decider::operator()() {
    const goal_lineage* chosen_gl = choose_goal();
    const size_t chosen_i = choose_candidate(chosen_gl);
    return std::make_pair(chosen_gl, chosen_i);
}

const goal_lineage* a01_decider::choose_goal() {
    // Get the goals to choose from
    std::vector<choice> goals;
    goals.reserve(gs.size());

    for (const auto& [gl, _] : gs)
        goals.push_back(gl);

    // Choose a goal to resolve
    const choice choice_a = sim.choose(goals);
    return std::get<const goal_lineage*>(choice_a);
}

size_t a01_decider::choose_candidate(const goal_lineage* chosen_gl) {
    // Get the candidates to choose from
    std::vector<choice> candidates;
    candidates.reserve(cs.count(chosen_gl));

    const auto candidate_range = cs.equal_range(chosen_gl);
    for (auto it = candidate_range.first; it != candidate_range.second; ++it)
        candidates.push_back(it->second);

    // Choose a candidate for the goal
    const choice choice_b = sim.choose(candidates);
    return std::get<size_t>(choice_b);
}
