#ifndef GOAL_STORE_HPP
#define GOAL_STORE_HPP

#include "defs.hpp"
#include "copier.hpp"
#include "bind_map.hpp"
#include "lineage.hpp"
#include "frontier.hpp"

struct goal_store : frontier<const expr::pred*> {
    goal_store(
        const database&,
        const goals&,
        trail&,
        copier&,
        bind_map&,
        lineage_pool&,
        expr_pool&
    );
    bool try_unify_head(const expr::pred* const&, const rule&, std::map<uint32_t, uint32_t>&);
    bool applicable(const expr::pred* const&, const rule&);
    std::vector<const expr::pred*> expand(const expr::pred* const&, const rule&) override;
#ifndef DEBUG
private:
#endif
    const database& db;
    trail& t;
    copier& cp;
    bind_map& bm;
    lineage_pool& lp;
    expr_pool& ep;  // needed to convert pred* → expr* for copier/unifier
};

#endif
