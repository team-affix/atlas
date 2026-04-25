#ifndef EVENT_AGGREGATOR_HPP
#define EVENT_AGGREGATOR_HPP

#include "defs.hpp"
#include "lineage.hpp"
#include "goal_store.hpp"
#include "candidate_store.hpp"
#include "frontier_watch.hpp"
#include "head_eliminator.hpp"
#include "cdcl_eliminator.hpp"

struct event_aggregator {
    event_aggregator(
        const database&,
        const goals&,
        bind_map&,
        expr_pool&,
        goal_store&,
        candidate_store&,
        lineage_pool&,
        cdcl&
    );
    bool operator()();
    bool pop_unit(const resolution_lineage*&);
    void resolve(const resolution_lineage*);
#ifndef DEBUG
private:
#endif
    std::function<void(const goal_lineage*)> goal_inserted_callback();
    std::function<void(const resolution_lineage*)> goal_resolved_callback();

    lineage_pool& lp;
    candidate_store& cs;

    bool conflict_register;
    std::queue<const resolution_lineage*> unit_queue;
    frontier_watch fw;
    cdcl_eliminator ce;
    head_eliminator he;
};

#endif
