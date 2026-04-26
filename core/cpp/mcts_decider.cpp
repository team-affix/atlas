#include "../hpp/mcts_decider.hpp"
#include "../hpp/locator.hpp"

mcts_decider::mcts_decider() :
    cs(locator::locate<candidate_store>(locator_keys::inst_candidate_store)),
    sim(locator::locate<monte_carlo::simulation<choice, std::mt19937>>(locator_keys::inst_mcts_sim)) {
}

std::pair<const goal_lineage*, size_t> mcts_decider::operator()() {
    const goal_lineage* chosen_gl = choose_goal();
    const size_t chosen_i = choose_candidate(chosen_gl);
    return std::make_pair(chosen_gl, chosen_i);
}

const goal_lineage* mcts_decider::choose_goal() {
    // Get the goals to choose from
    std::vector<choice> goals;
    goals.reserve(cs.get().size());

    // Convert the goals to choices
    for (auto it = cs.get().begin(); it != cs.get().end(); ++it)
        goals.push_back(it->first);

    // Choose a goal to resolve
    const choice choice_a = sim.choose(goals);
    return std::get<const goal_lineage*>(choice_a);
}

size_t mcts_decider::choose_candidate(const goal_lineage* chosen_gl) {
    // Get the candidates to choose from
    std::vector<choice> candidates;
    candidates.reserve(cs.get().at(chosen_gl).size());

    // Convert the candidates to choices
    for (size_t rule_id : cs.get().at(chosen_gl))
        candidates.push_back(rule_id);

    // Choose a candidate for the goal
    const choice choice_b = sim.choose(candidates);
    return std::get<size_t>(choice_b);
}
