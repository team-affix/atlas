#ifndef GOAL_ACTIVATOR_HPP
#define GOAL_ACTIVATOR_HPP

#include "../interfaces/i_goal_activator.hpp"
#include "../interfaces/i_frontier.hpp"
#include "../interfaces/i_candidate_activator.hpp"
#include "../interfaces/i_candidate_deactivator.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_goal_initializer.hpp"
#include "../interfaces/i_candidate_initializer.hpp"
#include "../interfaces/i_goal_factory.hpp"

struct goal_activator : i_goal_activator {
    goal_activator(
        const i_database&,
        i_lineage_pool&,
        i_frontier&,
        i_candidate_activator&,
        i_candidate_deactivator&,
        i_goal_initializer&,
        i_candidate_initializer&,
        i_goal_factory&
    );
    bool activate(const goal_lineage*) override;
private:
    const i_database& db;
    i_lineage_pool& lp;
    i_frontier& frontier;
    i_candidate_activator& candidate_activator;
    i_candidate_deactivator& candidate_deactivator;
    i_goal_initializer& goal_initializer;
    i_candidate_initializer& candidate_initializer;
    i_goal_factory& goal_factory;
};

#endif
