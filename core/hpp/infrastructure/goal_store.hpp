#ifndef GOAL_STORE_HPP
#define GOAL_STORE_HPP

#include "copier.hpp"
#include "bind_map.hpp"
#include "lineage.hpp"
#include "frontier.hpp"
#include "goal_expander.hpp"

struct goal_store : frontier<const expr*, goal_expander> {
    goal_store();
    bool applicable(const expr* const&, const rule&);
    goal_expander make_expander(const expr* const&, const rule&) override;
#ifndef DEBUG
private:
#endif
    trail& t;
    copier& cp;
    bind_map& bm;
    lineage_pool& lp;
};

#endif
