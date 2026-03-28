#include "../hpp/mcts_decider.hpp"

mcts_decider::mcts_decider(
    const candidate_store& cs,
    monte_carlo::simulation<choice, std::mt19937>& sim
)
    : cs(cs), sim(sim)
{}

std::pair<const goal_lineage*, size_t> mcts_decider::operator()() {
    const goal_lineage* chosen_gl = choose_goal();
    const size_t chosen_i = choose_candidate(chosen_gl);
    return std::make_pair(chosen_gl, chosen_i);
}

const goal_lineage* mcts_decider::choose_goal() {
    // Get the goals to choose from
    std::vector<choice> goals;
    goals.reserve(cs.size());

    // Convert the goals to choices
    for (auto it = cs.begin(); it != cs.end(); ++it)
        goals.push_back(it->first);

    // Choose a goal to resolve
    const choice choice_a = sim.choose(goals);
    return std::get<const goal_lineage*>(choice_a);
}

size_t mcts_decider::choose_candidate(const goal_lineage* chosen_gl) {
    // Get the candidates to choose from
    std::vector<choice> candidates;
    candidates.reserve(cs.at(chosen_gl).size());

    // Convert the candidates to choices
    for (size_t rule_id : cs.at(chosen_gl))
        candidates.push_back(rule_id);

    // Choose a candidate for the goal
    const choice choice_b = sim.choose(candidates);
    return std::get<size_t>(choice_b);
}
