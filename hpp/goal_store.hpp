#ifndef GOAL_STORE_HPP
#define GOAL_STORE_HPP

#include "defs.hpp"
#include "copier.hpp"
#include "bind_map.hpp"
#include "lineage.hpp"
#include "frontier.hpp"

struct goal_store : frontier<const expr*> {
    goal_store(
        const database&,
        const goals&,
        copier&,
        bind_map&,
        lineage_pool&
    );
#ifndef DEBUG
private:
#endif
    std::vector<const expr*> expand(const expr* const&, const rule&) override;
    const database& db;
    copier& cp;
    bind_map& bm;
    lineage_pool& lp;
};

#endif
