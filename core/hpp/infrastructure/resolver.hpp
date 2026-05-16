#ifndef RESOLVER_HPP
#define RESOLVER_HPP

#include <optional>
#include "../interfaces/i_resolver.hpp"
#include "../utility/state_machine.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_database.hpp"
#include "../interfaces/i_elimination_generator.hpp"
#include "../interfaces/i_elimination_backlog.hpp"
#include "../interfaces/i_goal_activator.hpp"
#include "../interfaces/i_goal_deactivator.hpp"
#include "../interfaces/i_candidate_activator.hpp"
#include "../interfaces/i_candidate_deactivator.hpp"
#include "../interfaces/i_goal_initializer.hpp"
#include "../interfaces/i_candidate_initializer.hpp"
#include "../interfaces/i_elimination_processor.hpp"

struct resolver : i_resolver {
    explicit resolver(size_t initial_goal_count);
    void init_resolve(const resolution_lineage*) override;
    void resume() override;
private:
    state_machine<void> resolve(const resolution_lineage*, size_t);
    state_machine<void> activate_goals(const resolution_lineage*, size_t);
    state_machine<void> activate_candidates(const goal_lineage*, goal&);
    state_machine<void> deactivate_goal(const goal_lineage*);
    state_machine<void> deactivate_candidates(const goal_lineage*, goal&);

    i_database& db;
    i_lineage_pool& lp;
    i_elimination_generator& eg;
    i_elimination_backlog& eb;
    i_goal_activator& goal_activator;
    i_goal_deactivator& goal_deactivator;
    i_candidate_activator& candidate_activator;
    i_candidate_deactivator& candidate_deactivator;
    i_goal_initializer& goal_initializer;
    i_candidate_initializer& candidate_initializer;

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
