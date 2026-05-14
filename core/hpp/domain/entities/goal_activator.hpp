#ifndef GOAL_ACTIVATOR_HPP
#define GOAL_ACTIVATOR_HPP

#include <vector>
#include "../interfaces/i_goal_activator.hpp"
#include "../interfaces/i_goal_factory.hpp"
#include "../interfaces/i_frontier.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_copier.hpp"
#include "../interfaces/i_factory.hpp"

struct goal_activator : i_goal_activator {
    explicit goal_activator(std::vector<const expr*> initial_exprs);
    void start_resolution(const resolution_lineage*) override;
    void activate(const goal_lineage*) override;
private:
    i_goal_factory& goal_factory_;
    i_frontier& frontier_;
    const i_database& db_;
    i_copier& copier_;
    i_unifier& unifier_;
    std::vector<const expr*> initial_exprs_;
    std::vector<const expr*> current_exprs_;
};

#endif
