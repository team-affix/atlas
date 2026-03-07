#ifndef A01_DECIDER_HPP
#define A01_DECIDER_HPP

#include "../mcts/include/mcts.hpp"
#include "a01_defs.hpp"

struct a01_decider {
    using choice = std::variant<const goal_lineage*, size_t>;
    a01_decider(
        a01_goal_store& gs,
        a01_candidate_store& cs,
        monte_carlo::simulation<choice, std::mt19937>& sim
    );
    std::pair<const goal_lineage*, size_t> operator()();
#ifndef DEBUG
private:
#endif
    const goal_lineage* choose_goal();
    size_t choose_candidate(const goal_lineage*);
    a01_goal_store& gs;
    a01_candidate_store& cs;
    monte_carlo::simulation<choice, std::mt19937>& sim;
};

#endif
