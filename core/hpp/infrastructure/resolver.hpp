#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include <optional>
#include "../interfaces/i_resolver.hpp"
#include "../utility/state_machine.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_elimination_generator.hpp"
#include "../interfaces/i_elimination_router.hpp"
#include "../interfaces/i_goal_activator.hpp"
#include "../interfaces/i_goal_deactivator.hpp"
#include "../interfaces/i_candidate_activator.hpp"
#include "../interfaces/i_candidate_deactivator.hpp"
#include "../interfaces/i_get_goal_candidates_size.hpp"
#include "../interfaces/i_goal_candidates_acceptor.hpp"

struct resolver : i_resolver {
    explicit resolver(size_t initial_goal_count);
    std::optional<sim_termination> resolve(const resolution_lineage*) override;
private:
    i_database& db;
    i_lineage_pool& lp;
    i_elimination_generator& eg;
    i_elimination_router& er;
    i_goal_activator& goal_activator;
    i_goal_deactivator& goal_deactivator;
    i_candidate_activator& candidate_activator;
    i_candidate_deactivator& candidate_deactivator;
    i_get_goal_candidates_size& gcs;
    i_goal_candidates_acceptor& gca;

    size_t initial_goal_count;
    
    // const resolution_lineage* parent_rl;
    // size_t body_size = 0;
    
    // bool activating_candidates = false;
    // const resolution_lineage* current_rl = nullptr;
    // size_t candidate_idx = 0;

    // bool activating_subgoals = false;
    // const goal_lineage* current_gl = nullptr;
    // size_t subgoal_idx = 0;

    // bool deactivating_candidates = false;

    // bool deactivating_goal = false;

    // bool finishing = false;

    std::optional<state_machine<void>> resolve_state_machine;
};

#endif
