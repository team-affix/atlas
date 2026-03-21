#ifndef GOAL_RESOLVER_HPP
#define GOAL_RESOLVER_HPP

#include "lineage.hpp"
#include "defs.hpp"
#include "copier.hpp"
#include "bind_map.hpp"
#include "lineage.hpp"
#include "goal_adder.hpp"

struct goal_resolver {
    goal_resolver(
        resolution_store&,
        goal_store&,
        candidate_store&,
        const database&,
        copier&,
        bind_map&,
        lineage_pool&,
        goal_adder&
    );
    void operator()(const goal_lineage*, size_t);
#ifndef DEBUG
private:
#endif
    resolution_store& rs;
    goal_store& gs;
    candidate_store& cs;
    const database& db;
    copier& cp;
    bind_map& bm;
    lineage_pool& lp;
    goal_adder& ga;
};

#endif
