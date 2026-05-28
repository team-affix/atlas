#ifndef ELIMINATION_ROUTER_HPP
#include "infrastructure/locator.hpp"

#define ELIMINATION_ROUTER_HPP

#include "interfaces/i_elimination_router.hpp"
#include "interfaces/i_deactivated_candidate_memory.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_insert_backlogged_elimination.hpp"
#include "interfaces/i_candidate_deactivator.hpp"

struct elimination_router : i_elimination_router {
    elimination_router(locator& loc);
    elimination_result route(const resolution_lineage*) override;
private:
    i_deactivated_candidate_memory& deactivated_candidate_memory;
    i_is_active_goal& is_active_goal;
    i_insert_backlogged_elimination& insert_backlogged_elimination;
    i_candidate_deactivator& candidate_deactivator;
};

#endif
