#ifndef ELIMINATION_ROUTER_HPP
#define ELIMINATION_ROUTER_HPP

#include "../interfaces/i_elimination_router.hpp"
#include "../interfaces/i_deactivated_candidate_memory.hpp"
#include "../interfaces/i_active_goals.hpp"
#include "../interfaces/i_elimination_backlog.hpp"
#include "../interfaces/i_candidate_deactivator.hpp"

struct elimination_router : i_elimination_router {
    elimination_router(
        i_deactivated_candidate_memory& dcm,
        i_active_goals& ag,
        i_elimination_backlog& eb,
        i_candidate_deactivator& cd);
    elimination_result route(const resolution_lineage*) override;
private:
    i_deactivated_candidate_memory& dcm;
    i_active_goals& ag;
    i_elimination_backlog& eb;
    i_candidate_deactivator& cd;
};

#endif
