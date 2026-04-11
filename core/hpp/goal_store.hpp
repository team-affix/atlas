#ifndef GOAL_STORE_HPP
#define GOAL_STORE_HPP

#include "defs.hpp"
#include "copier.hpp"
#include "bind_map.hpp"
#include "lineage.hpp"
#include "frontier.hpp"

struct goal_store : frontier<const expr*> {
    virtual ~goal_store() = default;
    goal_store(
        const database&,
        const goals&,
        trail&,
        copier&,
        bind_map&,
        lineage_pool&
    );
    std::optional<std::vector<const expr*>> expand(const expr* const&, const rule&) override;
#ifndef DEBUG
private:
#endif
    copier& cp;
    bind_map& bm;
};

#endif
