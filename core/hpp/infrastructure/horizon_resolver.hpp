#ifndef HORIZON_RESOLVER_HPP
#define HORIZON_RESOLVER_HPP

#include "infrastructure/locator.hpp"
#include "infrastructure/resolver.hpp"
#include "interfaces/i_resolver.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_get_goal_weight.hpp"
#include "interfaces/i_accumulate_grounded_weight.hpp"

struct horizon_resolver : i_resolver {
    horizon_resolver(locator& loc);
    bool resolve(const resolution_lineage*) override;
private:
    resolver& resolver_;
    i_get_rule& get_rule_;
    i_get_goal_weight& get_goal_weight_;
    i_accumulate_grounded_weight& accumulate_grounded_weight_;
};

#endif
