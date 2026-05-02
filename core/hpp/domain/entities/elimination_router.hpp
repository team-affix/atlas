#ifndef ELIMINATION_ROUTER_HPP
#define ELIMINATION_ROUTER_HPP

#include "../interfaces/i_elimination_router.hpp"
#include "../interfaces/i_active_goal_store.hpp"
#include "../interfaces/i_inactive_goal_store.hpp"
#include "../interfaces/i_elimination_backlog.hpp"
#include "../interfaces/i_active_eliminator.hpp"

struct elimination_router : i_elimination_router {
    elimination_router();
    void route(const resolution_lineage*) override;
private:
    i_active_goal_store& active_goal_store;
    i_inactive_goal_store& inactive_goal_store;
    i_elimination_backlog& elimination_backlog;
    i_active_eliminator& active_eliminator;
};

#endif
