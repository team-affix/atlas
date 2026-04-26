#ifndef MCTS_DECIDER_HPP
#define MCTS_DECIDER_HPP

#include "../../mcts/include/mcts.hpp"
#include "candidate_store.hpp"

struct mcts_decider {
    using choice = std::variant<const goal_lineage*, size_t>;
    mcts_decider();
    std::pair<const goal_lineage*, size_t> operator()();
#ifndef DEBUG
private:
#endif
    const goal_lineage* choose_goal();
    size_t choose_candidate(const goal_lineage*);
    const candidate_store& cs;
    monte_carlo::simulation<choice, std::mt19937>& sim;
};

#endif
